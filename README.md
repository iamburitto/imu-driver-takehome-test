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

The only other IMU I ever worked with produced like literally 1000 samples a second, and we had to average them on the FPGA even after setting the thing up to do its own internal averaging, and it was a whole thing with GNC since they only ran at 10Hz.

Anyway, for this prompt I am assuming 

Maybe you folks just wanted one thread to be triggered every 80ms, read and process as many IMU packets as possible, and die after a certain amount of time - I'm not sure.

But sometimes its nice to have one task with multiple threads - a consumer thread to parse in the background, and a more time-critical one (controlled more intensely when using Linux RT patch) that must execute at a pa

“execute the IMU parsing code every 80ms and broadcast the parsed results on the localhost network.”

Does that mean “store and broadcast everything” not "just broadcast the most up-to-date result?"?

Maybe. But I'm assuming the latter in my code:

- Thread A (UART parser running in background thread): parses packets as fast as bytes arrive, but doesn't hold onto all of them, just puts the latest one in a global struct
- Thread B (triggered every 80ms by scheduler): wakes up, grabs the latest parsed packet (shared memory with thread A and protected with mutexes), and broadcasts that to localhost
- This aligns with:
  - "resource-constrained" → no growing queues
  - Realistic embedded stuff (e.g., updating a housekeeping packet on a schedule)

- It might be simpler than that. Maybe they just want one thread to be triggered every 80ms and parse as much data as it can?
-

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
# Terminal 1: socat to link two PTYs (driver-to-emulator connection usually)
socat -d -d pty,raw,echo=0 pty,raw,echo=0

# Note the two PTY paths it prints - these are kinda like your virtual TX/RX UART ports.:
N PTY is /dev/pts/5 # choose this as RX (driver reads from it)
N PTY is /dev/pts/7 # choose this as TX (emulator writes to it)

# Terminal 2: Run C++ UART parser (device under test: basically IMU driver, but doesn't command, only receives telemetry)
./bin/imu_uart_parser /dev/pts/5 # driver reads from /dev/pts/5, finds sync, parses packets, and broadcasts them.

# Terminal 3: Send fake IMU packets (python emulator will do this later)
echo -ne '\x7f\xf0\x1c\xaf...' > /dev/pts/7

# Terminal 4: listen for UDP telemetry broadcast from driver (python emulator will do this later and validate telemetry it sent)
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

