#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <cstring>
#include <cstdint> // for fixed-width integer types

int configure_uart(const char* device);

int main() {
    const char* port = "/dev/tty1";  // default - replace with /dev/pts/X for testing
    
    if (argc > 1)
    {
        port = argv[1]; // override port with command line argument
    }

    std::cout << "Opening serial port: " << port << "\n";
    
    int fd = configure_uart(port);
    if (fd < 0) return 1;

    uint8_t buf[256];
    while (true) {
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n > 0) {
            printf("Read %zd bytes: ", n);
            for (ssize_t i = 0; i < n; ++i) {
                printf("%02X ", buf[i]);
            }
            printf("\n");
        } else {
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
