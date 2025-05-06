#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <cstring>
#include <cstdint> // for fixed-width integer types

int configure_uart(const char* device);

/*
    Pseudocode plan for the new main loop: UART + parser:

    main(argc, argv):
    - Read serial port path from command-line args (default to /dev/tty1)
    - Call configure_uart(path) → fd
    - Create a 1024-byte read buffer
    - Create a small sliding window or queue for sync detection
    - Loop:
        - read() from fd into buffer
        - for each byte:
            - Push byte into temporary buffer
            - Check if the last 4 bytes match sync pattern
                - If yes, check if you have at least 20 bytes
                - Call parse_imu_packet() on the 20-byte window
                - Print out result

        - This loop should only run until it finds valid packets.
        - Later I will plug the parsed result into a thread-safe shared struct.

*/

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


int main(int argc, char** argv) {
    const char* port = "/dev/tty1";  // default - replace with /dev/pts/X for testing
    
    if (argc > 1)
    {
        port = argv[1]; // override port with command line argument
    }

    printf("Opening serial port: %d\n", port);
    int fd = configure_uart(port);
    if (fd < 0)
    {
        return 1;
    }

    uint8_t buf[1024];
    while (true)
    {
        ssize_t nbytes_read = read(fd, buf, sizeof(buf));

        if (nbytes_read > 0)
        {
            printf("Read %zd bytes: ", nbytes_read);

            for (ssize_t i = 0; i < nbytes_read; ++i)
            {
                printf("%02X ", buf[i]);
            }
            printf("\n");
        }
        else
        {
            usleep(1000); // Small sleep to avoid busy-looping
        }
    }

    close(fd);
    return 0;
}

int configure_uart(const char* device) {

    // Open the serial port for read/write, no controlling terminal, synchronous writes
    int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        std::cerr << "Failed to open UART: " << strerror(errno) << "\n";
        return -1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof tty);

    if (tcgetattr(fd, &tty) != 0) {
        std::cerr << "Failed to get current UART config\n";
        return -1;
    }

    // Set baud rate (both input and output)
    cfsetospeed(&tty, B921600);
    cfsetispeed(&tty, B921600);

    // Raw mode: disable canonical input, echo, signals, etc.
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    // Disable output processing
    tty.c_oflag = 0;
    // No software flow control
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    // 8 data bits, no parity, 1 stop bit (8N1)
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    // Enable receiver, ignore modem control lines
    tty.c_cflag |= (CLOCAL | CREAD);

    // Set VMIN and VTIME so read() returns after 1 byte or timeout (100ms)
    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 1;

    // Apply the settings
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        std::cerr << "Failed to set UART config\n";
        return -1;
    }

    return fd;
}
