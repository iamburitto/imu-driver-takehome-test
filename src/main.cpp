#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h> // for open()
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

// TODO: put these somewhere better later
constexpr size_t UART_BUFFER_SIZE = 1024; // TODO: calculate how big it actually needs to be at this baud rate for 20 byte packets and running every 80ms
constexpr size_t PACKET_SIZE  = 20;
constexpr uint8_t SYNC_BYTES[4]   = {0x7F, 0xF0, 0x1C, 0xAF};

int main(int argc, char** argv)
{
    // read serial port path from command-line args (default to /dev/tty1)
    const char* port = (argc > 1) ? argv[1] : "/dev/tty1";
    
    // call configure_uart(port) -> fd
    printf("Opening serial port: %d\n", port);
    int fd = configure_uart(port);
    if (fd < 0)
    {
        perror("UART open failed");
        return 1;
    }

    // Create a read buffer
    std::array<uint8_t, UART_BUFFER_SIZE> buffer{}; // std::array doesn't use the heap - same footprint as uint8_t buffer[UART_BUFFER_SIZE];

    // create a small sliding window for sync detection

     

    


    

    

}

int main_initial_working_version(int argc, char** argv) {
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

int configure_uart(const char* device)
{
    // Open the serial port for read/write, no controlling terminal, synchronous writes
    // TODO: read man termios to actually understand these options that you got from stackoverflow
    int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        std::cerr << "Failed to open UART: " << strerror(errno) << "\n";
        perror("Failed to open UART: %s");
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
