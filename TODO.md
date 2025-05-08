# TODO 

## ✅ TODO / Progress Tracker

---

### Phase 1: Core Infrastructure (✅ done, ~3 hrs total)
- [x] Set up Git repo and `.gitignore`
- [x] Create initial CMake build system
- [x] Integrate Catch2 using `FetchContent`
- [x] Set up `test/` directory with `unit_tests` target
- [x] Create `imu_parser.[hpp|cpp]` with clean packet struct
- [x] Create `fendian.hpp` with endian-safe float helpers
- [x] Validate build and linker setup with dummy test

---

### Phase 2: Parser Development (✅ done, ~2–3 hrs total)
- [x] Define and implement `parse_imu_packet()`
- [x] Use `be32toh()` for integer field conversion
- [x] Use `f_be32toh()` to convert floats from big-endian
- [x] Validate all fields in parser against known packet layout
- [x] Write unit tests using realistic byte buffer
- [x] Add sender-style test using `f_htobe32()` for float fields
- [x] Confirm all unit tests pass

---

### 🧲 Phase 3: UART Integration (~2.5–3 hrs)
- [x] Replace `/dev/tty1` hardcoded path with command-line arg
- [x] Use `socat` to create linked PTY pair for testing
- [x] Read from serial port in `imu_parser`
- [x] Buffer incoming data and detect sync words
- [x] Extract 20-byte packet and call `parse_imu_packet()`
- [x] Print parsed values to stdout for validation

---

### ⏲️ Phase 4: Timed Execution & UDP Broadcast (~2–2.5 hrs)
- [x] Make main.c that compiles with 2 thread function stubs (1 parser, 1 80ms broadcaster)
- [x] Launch both parser and broadcaster thread that executes every 80ms
- [x] Share latest parsed packet in thread-safe manner (with `std::mutex`)
    - Actually, does it need to be the latest one? Or should we just be getting all of them?
- [ ] Format parsed data into a simple UDP payload
- [ ] Send it to `localhost` over UDP on a fixed port
- [ ] Confirm with netcat or script that data is being sent

---

###  Phase 5: Python Simulator (~1.5–2 hrs)
- [x] Generate 20-byte IMU packets with struct.pack()
- [x] Send packet over serial (using `pyserial`) every 20ms
- [x] Listen on UDP for the broadcast from the C++ parser
- [x] Decode UDP payload and print or validate results
- [x] Use `socat` to create virtual port pair for testing (looped to C++ side)

---

### Phase 6: Polish & Submission (~1 hr)
- [x] Change telemetry names to include 'rdps' units since that's what the prompt asks for
- [x] Write usage instructions in `README.md`
- [X] Write next steps in `README.md`
- [x] Add example commands
- [x] Final code comment review and formatting
- [x] Run end-to-end test (socat + simulator + parser + UDP)
- [x] Tag final commit, zip if needed, add a video/screenshot maybe, submit

---
