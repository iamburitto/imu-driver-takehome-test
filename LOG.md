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
- [ ] Read from serial port in `imu_uart_parser`
- [ ] Buffer incoming data and detect sync word
- [ ] Extract 20-byte packet and call `parse_imu_packet()`
- [ ] Print parsed values to stdout for validation

---

### ⏲️ Phase 4: Timed Execution & UDP Broadcast (~2–2.5 hrs)
- [ ] Launch thread that executes every 80ms
- [ ] Share latest parsed packet in thread-safe manner (with `std::mutex`)
- [ ] Format parsed data into a simple UDP payload
- [ ] Send it to `localhost` over UDP on a fixed port
- [ ] Confirm with netcat or script that data is being sent

---

### 🧪 Phase 5: Python Simulator (~1.5–2 hrs)
- [ ] Generate 20-byte IMU packets with struct.pack()
- [ ] Send packet over serial (using `pyserial`) every 20ms
- [ ] Listen on UDP for the broadcast from the C++ parser
- [ ] Decode UDP payload and print or validate results
- [ ] Use `socat` to create virtual port pair for testing (looped to C++ side)

---

### 🧼 Phase 6: Polish & Submission (~1 hr)
- [x] Change telemetry names to include 'rdps' since that's what the prompt asks for
- [ ] Write usage instructions in `README.md`
- [ ] Add example `socat` and simulator commands
- [ ] Final code comment review and formatting
- [ ] Run end-to-end test (socat + simulator + parser + UDP)
- [ ] Tag final commit, zip if needed, submit

---

### 🕒 Remaining Time Estimate:
**~8–10 hours total** depending on breaks and pace  
(even faster with scaffolds or if simulator is lightweight)

---

### 🎯 Pro Tips:
- Focus on UART → parser → UDP as your “critical path”
- Simulator can be light—just needs to show understanding
- You’ve already built the hardest pieces!

- [std::array dynamic memory: no thanks!](https://www.modernescpp.com/index.php/std-array-dynamic-memory-no-thanks/)

- Pseudocode plan for separate 80ms thread later
```c++
/*
    Pseudocode plan for once the above is working ok: Thread-safe communication between UART and 80ms thread
    
    How it should work:

    - I need one thread reading from the UART and parsing packets
    - I need a second thread to wake up every 80ms and broadcast the most recent valid packet on localhost (double check that was the instructions)
    - The two threads have to share a single ImuPacket struct instance (latest_packet)

    Problems/race conditions maybe:

    - UART thread: might be writing to latest_packet
    - 80ms thread: might be reading from latest_packet at the same time
    - If this happens without synchronization the data could get corrupted

    Solution probably:

    - std::mutex to protect access to latest_packet
    - uart thread locks and updates latest_packet and unlocks
    - 80ms thread locks and reads latest_packet and sends it to localhost, where the python script picks it up

    - ? It might not actually be the latest packet, might just be all of them? IDK.
    
    More advanced solution: std::atomic<std::shared_ptr<ImuPacket>> (if using smart pointers but not necessary probably)

    Another solution: double buffering (but probably not needed here yet)

    Pseudocode for thread safety:

    global mutex latest_mutex
    global ImuPacket latest_packet

    UART thread:
        - When new packet is parsed:
            lock(latest_mutex)
            latest_packet = parsed_result
            unlock(latest_mutex)

    80ms thread:
        loop every 80ms:
            lock(latest_mutex)
            copy = latest_packet
            unlock(latest_mutex)
            send copy over UDP

    This gives both threads safe and consistent access to the same data without stepping on each other I thiiiink

*/
```
- Ugh its like almost 7pm, been working on fixing a blue screen of death
- Working on updating the main loop to actually parse packets out of the stream now rather than just printing them.
  - [ ] Read from serial port in `imu_uart_parser`
  - [ ] Buffer incoming data and detect sync word
  - [ ] Extract 20-byte packet and call `parse_imu_packet()`
  - [ ] Print parsed values to stdout for validation
- Right now we are only going to read one byte at a time - just easier to debug that way.
- In the future, its better to read multiple bytes for these reasons:
  - Data may arrive in bursts (real-world UARTs do that)
  - The hardware UART FIFO is nonzero (common in SoCs)
  - I want fewer syscalls (read() per byte gets expensive)
  - I've debugged the edges of your framing logic
- For that more "real-world" driver, I'd write a version that reads up to N bytes, uses a fixed-size buffer with read_index and write_index, and defragments when nearing capacity, kinda like this:
  - UART driver loop:
    - if write_index within spitting distance of end of buffer:
        - shift unprocessed bytes to beginning
        - reset read_index to zero and write_index to next free space after read index
      - read(fd, &buffer[write_index], available_space)
      - advance write_index (should point at next free space)
    - Parser loop:
      - while read_index < write_index - 20:
          - check for sync at buffer[read_index]
          - if found and enough bytes: parse and advance read_index
          - if not found: read_index++
        
- But for now I'm going to keep it easy.
- 

