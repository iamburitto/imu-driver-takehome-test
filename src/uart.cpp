#include "uart.hpp"

int configure_uart(const char* device)
{
    // Open the serial port for read/write, no controlling terminal, synchronous writes
    // TODO: read man termios to actually understand these options that I finaegled together from stack overflow
    printf("Opening serial port: %s\n\n", device);
    int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        perror("Failed to open UART");
        return -1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof tty);

    if (tcgetattr(fd, &tty) != 0) {
        perror("Failed to get current UART config");
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

    // Set VMIN and VTIME so read() returns after 1 byte or timeout
    // 100ms timeout default I Think? might cause problem later, we might want this to be different if not just reading 1 byte at a time
    // probably depends a lot on UART IP and/or hardware depending on what this is running on, gotta fiddle, gotta hook up saleae
    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 1;

    // Apply the settings
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("Failed to set UART config");
        return -1;
    }

    return fd;
}
