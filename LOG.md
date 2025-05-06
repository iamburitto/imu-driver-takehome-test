# Just a place for me to put my thoughts

## 5/5/2025
- despite the late start, I've got a cmake project with unit testing revved up, and an initial configure_uart() method for the serial port at /dev/ttyS1.
- This [telemetry tutorial](https://web.archive.org/web/20160918103502/http://www2.l-3com.com/trf/telemetry_tutorial/telemetry_tutorial.pdf) looks worth reading
- Neat, turns out you can print a ssize_t with %zd, a size_t with %zu, int with %d, unsigned int with %u, long with %ld, and unsigned long with %lu
- ++i better in tight loops: find out why, look at old C book manual thingy
- Got socat to connect two virtual terminals and got bytes across!
- Next steps in parallel:
  - Add a framing & parsing function:
    - Start buffering bytes and scan for the 0x7F F0 1C AF header
    - When found, grab 20 bytes and parse packet_count, x_rate, y_rate, z_rate
    - Call parse_imu_packet() function
- Write unit tests for parsing logic (first?)
  - Confirm parser works even without UART
  - Test little-endian vs network (big endian) byte order conversion
- Start a separate thread for the `80ms` loop (not yet time-sensitive)
  - Set up a placeholder std::thread that wakes every 80ms
  - Later it will broadcast the latest parsed result over UDP or whatever (localhost like the prompt says)
- Let's go for the framing and parsing function first
- This looks interesting, but I'm getting ahead of myself: https://www.yolinux.com/TUTORIALS/Endian-Byte-Order.html

## 5/6/2025

- OK this is where I'm at:
## ✅ TODO / Progress Tracker

### ✨ Phase 1: Core Infrastructure
- [x] Set up Git repo and `.gitignore`
- [x] Create initial CMake build system
- [x] Integrate Catch2 using `FetchContent`
- [x] Set up `test/` directory with `unit_tests` target
- [x] Create `imu_parser.[hpp|cpp]` with clean packet struct
- [x] Create `fendian.hpp` with endian-safe float helpers
- [x] Validate build and linker setup with dummy test

### 🔎 Phase 2: Parser Development
- [x] Define and implement `parse_imu_packet()`
- [x] Use `be32toh()` for integer field conversion
- [x] Use `f_be32toh()` to convert floats from big-endian
- [x] Validate all fields in parser against known packet layout
- [x] Write unit tests using realistic byte buffer
- [x] Add sender-style test using `f_htobe32()` for float fields
- [x] Confirm all unit tests pass

### 🧲 Phase 3: UART Integration
- [ ] Replace `/dev/tty1` hardcoded path with command-line arg
- [ ] Use `socat` to create linked PTY pair for testing
- [ ] Read from serial port in `imu_uart_parser`
- [ ] Buffer incoming data and detect sync word
- [ ] Extract 20-byte packet and call `parse_imu_packet()`
- [ ] Print parsed values to stdout for validation

### ⏲️ Phase 4: Timed Execution & UDP Broadcast
- [ ] Launch thread that executes every 80ms
- [ ] Share latest parsed packet in thread-safe manner
- [ ] Format parsed data into a simple UDP payload
- [ ] Send it to `localhost` over UDP on a fixed port

### 🧼 Phase 5: Polish & Delivery
- [ ] Write usage instructions in `README.md`
- [ ] Final pass on code comments and structure
- [ ] Add example command lines

```c
Estimated time remaining by phase:
✅ Phase 3: UART Integration (~2.5–3 hrs)
Arg parsing and UART setup: 15–30 min

Reading bytes + sync framing + parser call: 1–1.5 hrs

Printing output for validation: 30 min

Testing with socat: 30 min

✅ Phase 4: 80ms Thread + UDP (~2–2.5 hrs)
Thread loop + sleep_for: 30–45 min

std::mutex-protected shared ImuPacket: 45 min

UDP sender using sendto() + sockaddr: 30–45 min

Validation + polish: 30 min

✅ Phase 5: Polish & Submission (~1 hr)
README, .gitignore, usage guide: 30–45 min

Final integration test & cleanup: 15–30 min
```