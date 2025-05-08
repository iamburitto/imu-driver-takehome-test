# IMU UART Parser - Takehome Assignment

## Prompt

An IMU outputs a byte stream at `921600` baud with a packet definition described below:

| Byte offset 	| Size 	| Type 	| Description 	|
|---	|---	|---	|---	|
| 0 	| 1 	| Raw byte 	| Start of frame byte - 0x7F 	|
| 1 	| 1 	| Raw byte 	| Start of frame byte - 0xF0 	|
| 2 	| 1 	| Raw byte 	| Start of frame byte - 0x1C 	|
| 3 	| 1 	| Raw byte 	| Start of frame byte - 0xAF 	|
| 4 	| 4 	| Unsigned Integer 	| Packet count 	|
| 8 	| 4 	| Float 	| X-axis gyro rate 	|
| 12 	| 4 	| Float 	| Y-axis gyro rate 	|
| 16 	| 4 	| Float 	| Z-axis gyro rate 	|

- Assume some flavor of Linux running
- Assume data is formatted according to `IEEE-754`
- Assume network (big endian) byte order

1. Write the C++ code to parse data over a UART (`/dev/tty1`) and decommutate the byte stream into the four fields: 

- Packet_Count 
- X_Rate_rdps 
- Y_Rate_rdps 
- Z_Rate_rdps

2. What modifications, if any, would you need to make to the code from 1) if the Linux host was a little endian processor? Describe those modifications.

No modifications are necessary (if we can assume that the IMU packets themselves really are big-endian.)

It shouldn't matter if the linux host is running on a little endian processor or a big endian processor -  `htobe32()` and `be32toh()` check endianness: they swap bytes if the host is little endian, and do nothing at all if the host is big endian. That makes them portable, though I'm sure there are weird exceptions.

It works like this:

- Packet sender uses `htobe32()` to convert host byte order to network byte order (big endian) for each multi-byte field 
- Packet receiver uses `be32toh()` to convert network byte order (big endian) to host byte order for each multi-byte field.

Usually its more of a pain in the a** for floats and doubles, and the IMU packet in this case has floats.
Since we are assuming `IEE-754` floats, I cheat by using unions to pretend the floats are `uint32_t` so I can still use `htobe32()` and `be32toh()` - see my wrapper functions `f_htobe32()` and `f_be32toh()` in `include/fendian.hpp`.

`NOTE`: I've been assuming that the IMU packet protocol itself is all big-endian.

If that wasn't true, and it turns out that the IMU fields can be little-endian or a mix of big-endian and little-endian (gross but common), then we'd have to use `htole32()` and `le32toh()` in addition or instead, and I'd have to add `f_htole32()` and `f_le32toh()` to `fendian.hpp` for the floats.

I'd probably want to add 8-byte double versions of all 4 wrapper functions in the future too (`d_htole32()`, etc.), and extend it as necessary for other weird data types.

3. Write a C++ Linux program designed to execute in a resource constrained, multi-threaded 
environment that will execute the IMU parsing code every `80ms` and broadcast the parsed results on the localhost network.

- “execute the IMU parsing code every 80ms and broadcast the parsed results on the localhost network.”
- Does that mean “parse and broadcast as many packets as possible once every 80ms”, or "parse all packets but drop most on the floor for now and only broadcast the latest packet every 80ms"?
- Do we want one thread triggered every 80ms to read and process and forward all the packets?
- Or multiple threads - one to parse packets in the background and one to broadcast?
- Do you want them all broadcasted, or only the latest?

I wasn't sure. I decided to assume the latter and do two threads for fun - could always combine them into one 80ms thread instead, so here's what I made:

- "producer" == Thread A (UART parser running in background thread): parses packets as fast as bytes arrive but only stores the most recent one in a global struct (shared memory with thread B)

- "consumer" == Thread B (triggered every 80ms by scheduler): wakes up, grabs the latest parsed packet (shared memory with thread A), and broadcasts that to localhost

- This aligns with:
  - "resource-constrained" → no growing queues
  - Realistic embedded stuff (e.g., updating a housekeeping packet on a schedule)

NOTE: This could be done all sorts of ways depending on the architecture. In reality, some threads are time critical and tightly controlled with extra knobs and buttons (CPU, start time, end time, priority, etc etc), and we'd probably want Linux RT.

Other considerations:

If it was a real IMU I'd want to know how many samples per second this IMU produces and how noisy it is - otherwise I don't really know how much data I'm dealing with, only the maximum possible number of packets from the baud rate.

The only other IMU I ever worked with produced 1000 samples a second and was too noisy even after configuring the IMU to average its data internally, so we had to average/process it again to get rid of the noise and get it down to 10Hz for GNC's 10Hz flight control model (Nyquist theorem stuff.) There was too much latency to do that in software for the flight computer, so I did it on the SOC's FPGA.

GNC would probably get only the most recent telemetry point when the FCSW model steps, and I'd likely have configurable data rates for all other downlink/storage destinations - high rates for gathering data and debugging, lower data rates for historical telemetry, and all configurable to different rates for different situations.

It all depends on what mission operations and GNC folks want.

4. In Python, design a simple simulator/tester to drive your IMU parser. 

- The simulator should send a string of bytes in the proper format over the UART for your parser to accept
-  The data contained in the byte string can be generated in any way of your choosing to best test the parser.
- Subsequently, it should then read the broadcasted results from 3) on the localhost network to validate the output.

### Project Structure

- `src/` → C++ source code
- `include/` → C++ headers
- `test/` → catch2 unit tests, future test scripts (e.g. Python simulator)
- `build/` → CMake build output (gitignored)

### How to build

```bash
mkdir -p build
cd build
cmake ..
make
```

### How to build and run just unit tests

```bash
cd build
cmake ..
make unit_tests
./bin/unit_tests
Randomness seeded to: 1089831589
===============================================================================
All tests passed (1 assertion in 1 test case)
```

### How to test `imu_uart_parser` using `socat` to create two linked virtual serial ports

A `pty` is a `pseudoterminal` (fake serial port) when we don't have access to an actual hardware terminal serial port (`tty`).

We are creating two linked virtual serial ports like this:

```bash
/dev/pts/5  <==>  /dev/pts/7
[imu_uart_parser]      [python or echo]
```

Anything written to one side will show up on the other side.

To test this we need 3 terminal windows.

In a nutshell:

```bash
# Terminal 1: socat to link two PTYs (virtual serial ports TX/RX)
socat -d -d pty,raw,echo=0 pty,raw,echo=0

# Note the two PTY paths it prints
N PTY is /dev/pts/5 # choose this as RX (driver reads from it)
N PTY is /dev/pts/7 # choose this as TX (emulator writes to it)

# Terminal 2: Run C++ UART parser (device under test: basically IMU driver, but doesn't command, only receives telemetry)
./bin/imu_uart_parser /dev/pts/5 # driver reads from /dev/pts/5, finds sync, parses packets, and broadcasts them.

# Terminal 3: Send fake IMU packets (python emulator will do this later)
echo -ne '\x7f\xf0\x1c\xaf...' > /dev/pts/7

# Terminal 4: listen for UDP telemetry broadcast from driver (python emulator will do this later)
netcat -ul 9000
```

In more detail:

In `terminal 1`: create two linked virtual serial ports:

```bash
# Use `socat` to create two virtual terminals and connect them together.
$ socat -d -d pty,raw,echo=0 pty,raw,echo=0
2025/05/05 21:51:00 socat[82065] N PTY is /dev/pts/5
2025/05/05 21:51:00 socat[82065] N PTY is /dev/pts/7 
2025/05/05 21:51:00 socat[82065] N starting data transfer loop with FDs [5,5] and [7,7]
```

`socat` arguments in this case:

- `-d -d` prints verbose info (like `PTY` names)
- `pty` allocates a pseudoterminal (like `/dev/pts/3`)
- `raw` means no line buffering or special character handling, just raw bytes
- `echo=0` means don't echo back what you send (avoids byte duplication)

In `terminal 2`: connect `imu_uart_parser` to one side:

```bash
# run the imu_uart_parser program with the first PTY
$ ./build/bin/imu_uart_parser /dev/pts/5
Opening serial port: /dev/pts/5
```

In `terminal 3`: write simulated IMU data bytes to the other side:

```bash
# write simulated data bytes to the first PTY by echoing them to the second PTY:
$ echo -ne '\x7f\xf0\x1c\xaf\x00\x00\x00\x01\x41\x20\x00\x00\x41\xa0\x00\x00\x42\x20\x00\x00' > /dev/pts/7
```

The simulated IMU data bytes will show up in `terminal 1`:

```bash
Read 20 bytes: 7F F0 1C AF 00 00 00 01 41 20 00 00 41 A0 00 00 42 20 00 00
```

In `terminal 4`: listen for UDP telemetry broadcast

