# IMU UART Parser - Takehome Assignment

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

2. What modifications, if any, would you need to make to the code from 1. if the Linux host was a little endian processor? Describe those modifications.

3. Write a C++ Linux program designed to execute in a resource constrained, multi-threaded 
environment that will execute the IMU parsing code every `80ms` and broadcast the parsed results on the localhost network.

4. In Python, design a simple simulator/tester to drive your IMU parser. 

- The simulator should send a string of bytes in the proper format over the UART for your parser to accept
-  The data contained in the byte string can be generated in any way of your choosing to best test the parser.
- Subsequently, it should then read the broadcasted results from `3` on the localhost network to validate the output.

## Project Structure

`src/` → C++ source code
`include/` → C++ headers
`test/` → Future test scripts (e.g. Python simulator)
`build/` → CMake build output (usually gitignored)

## How to build

```bash
mkdir -p build
cd build
cmake ..
make
```

## How to build and run unit tests

```bash
cd build
cmake ..
make unit_tests
./bin/unit_tests
```

## How to Run (with virtual UART)

Use `socat` to create virtual terminals:

```bash
socat -d -d pty,raw,echo=0 pty,raw,echo=0
```

Use one `/dev/pts/X` for the C++ parser, and the other for sending simulated data via Python or echo.

