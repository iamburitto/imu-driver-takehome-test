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

- [std::array dynamic memory: no thanks!](https://www.modernescpp.com/index.php/std-array-dynamic-memory-no-thanks/)
- Pseudocode plan for separate 80ms thread later
- Eventually we're going to execute the IMU parsing loop every `80ms` and broadcast the parsed results on the localhost network
- But step 1 is just to make the loop parse and print packets out of the stream.
- So we're gonna make the output look like this:

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
- Good guidelines for error:
```c++
  // Use perror for immediate, simple error reporting
  perror("Error opening file");
  // Use printf + strerror for custom formatting or delayed reporting
  fprintf(stderr, "Failed to open file: %s\n", strerror(errno));
```

- Ugh now its 11:54pm
- But it actually works now - its parsing through the stream and finding packets (at least the first packet), though it needs test coverage.
- The next step is to figure out what the prompt is asking me for.
- Like does it want me to make one task that the scheduler triggers every 80ms to broadcast the latest packet to localhost, while a second thread runs in the background processing as fast as possible? OR does it want me to broadcast the entire queue of packets every 80ms?
- “execute the IMU parsing code every 80ms and broadcast the parsed results on the localhost network.”
- Does that mean a single task with multiple threads (1 background thread and 1 80ms thread that broadcasts the latest?)
- Like the scheduler runs every 80ms, not the UART parser, and it broadcasts whatever was parsed most recently?
- Or does it mean “store and broadcast everything” not "just broadcast the most up-to-date result?"?
- idk I'm making no sense, time to sleep.

Assumptions:

- Thread A (UART parser running in background thread): runs as fast as bytes arrive, updates the latest good packet
- Thread B (triggered every 80ms by scheduler): wakes up, grabs the latest parsed packet (shared memory with thread A and protected with mutexes), and broadcasts that to localhost
- This aligns with:
  - "resource-constrained" → no growing queues
  - Realistic embedded stuff (e.g., updating a housekeeping packet on a schedule)

- It might be simpler than that. Maybe they just want one thread to be triggered every 80ms and parse as much data as it can?
- The only other IMU I ever worked with produced like literally 1000 samples a second, and we had to average them on the FPGA to get it to make sense at all even after setting the thing up to do its own internal averaging, and simulating the noise was a huge pain
- I would ask a GNC person what they wanted

## 5/7/2025

- Another late start!
- Plan for today is to add the two threads.
- Its still possible that's not what they intended, but gonna go for it anyway cause I'm rusty with threading and it'll be fun
- Thread A (UART parser running in background thread): runs as fast as bytes arrive, updates the latest good packet
- Thread B (triggered every 80ms by scheduler): wakes up, grabs the latest parsed packet (shared memory with thread A and protected with mutexes), and broadcasts that to localhost
- Got the two threads implemented and compiling at the library, now heading to the gym to climb between code
- Opening UDP socket, gym open for one more hour
- considered making cntrl-c work ok for sockets but its gonna take too long tonight, so here's what to look at while testing:
  - check for open file descriptors: `lsof -p <pid>`
  - check for max number of file descriptors per process: `ulimit -n`
  - watch for "too many open files" errors (leaking sockets), probs not important for this test
- Updated README to explain the endian business and assumptions/background for IMU choices better
- Had to install python on this WSL2 ubuntu instance (I don't have my old awesome debian one anymore)
- Didn't set up venv because I'm tired, but on a fresh system I 100% would
```bash
sudo apt update
sudo apt upgrade
sudo apt install python3 python3-pip
python3 --version
sudo apt install python3-venv
```
- I'd go watch [this video](https://www.youtube.com/watch?v=dC1A8t-G3tw) if I wanted to get venv with python2/3 working well together

## 5/8/2025 5:34AM

- IT WORKS see end of README for details and test output, I'll include a video too, hotdamn 🥳💃🎊😅😴



