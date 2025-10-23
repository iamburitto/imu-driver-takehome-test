# Next Steps

- programatically open the ptys instead of doing with with socat maybe, or put it in a script
- programatically verify both sides match in the simulator script (visually they do)
- clean and simplify, get rid of extraneous stuff that might confuse people before a real code review
- clean up C code and remove unnecessary stuff, make it more C-like
  - I prefer C+/- , I hate seeing all those namespace :: everywhere
  - I find a lot of C++ code a lot harder to debug at a low level most of the time
  - I really like MISRA naming
  - I like using OO stuff only when necessary
  - I've got a big list of preferences to compromise on, and I'm open to new ideas (hence the std::pthreads in this project)
- probably would end up changing parsing code to parse more than 1 byte at a time and use a state machine
  - see the long comment at the end of `imu_parser.cpp` for details about what I'd like to change or what might have to change and why and possible architecture alternatives
- If its on an SOC, I'd start by getting the firmware and petalinux build revved up, which I'd really like the chance to do myself
- get it running on a ZCU/hardware as soon as possible, ideally with the real thing, but at least with a saleae - no way in heck is it gonna work on hardware and petalinux the first time, and that's the fun part (and what I'd prefer to start with if at all possible, though this was fun)
- do a deep dive on the UART serial port settings... get this all working on a ZCU102, but I doubt this is the final version of the uart config code - probably depends a lot on UART IP in FPGA and/or hardware, and whether I keep parsing 1 byte at a time, and what hardware (FPGA vs microcontroller, what processor, etc, what avionics, what electrical standard) depending on what this is running on
- figure out the actual math and timing diagram for the threads with baud rate to estimate what the limits are and how big the buffers should be using actual IMU docs
- Um, is there really not a CRC, or at least some kind of checksum? Radiation and even just regular old packet corruption is a thing...
- regardless of CRC/checksum/anything, add more unit tests to verify still works in all kinds of weird packet corruption situations and edge cases, not just with valid packets like I'm doing now
- add .gitlab-ci.yml to run unit tests
- add a little tool to print test coverage percentage and missing lines for methods/files (nice to have for later, but also makes stuff take longer because not having full coverage itches your brain)
- use regular p_threads instead of std::thread instead (maybe)
  - I think it would give more control over timing, work better with legacy code, lots of other reasons probably
  - I just wanted to try something new, but not sure if there's dynamic memory allocation in there
  - Or maybe its fine, but I'd want to try both ways anyway
- figure out how to run in RT Linux and actually schedule the threads - we probably need an executor, and a config to tweak them easily
- figure out some way to visualize all the threads - STM32 microprocessors + FreeRTOS + SEGGER JLink & other free SEGGER tools figured that **** out SUPER well, beautiful timing diagrams - idk why we don't have something like that for linux yet, probably just cause hardware is more complicated?
- all the other stuff to make imu_parser into an actual driver, too much to list right now
- probably plenty of stuff to do to make emulator side work better (I've never done emulators except for the FPGA part, just flight code)

- Practice explaining key parts aloud:
- termios settings
- why std::mutex instead of atomic or queue
- why UDP
- why one packet per 80ms (math)
- how you'd test or verify robustness (CRCs, packet loss, timing)
- Quick mental refresher on read(), sendto(), mutex, snprintf(), struct.pack() before the interview


