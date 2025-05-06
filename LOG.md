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
- 