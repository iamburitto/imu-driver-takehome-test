#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h> // for open()
#include <termios.h>
#include <cstring>
#include <cstdint> // for fixed-width integer types
#include <cstdlib> // for size_t
#include "imu_parser.hpp"

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
constexpr size_t UART_BUFFER_CAPACITY = 1024; // TODO: calculate how big it actually needs to be at this baud rate for 20 byte packets and running every 80ms
constexpr size_t PACKET_SIZE  = 20;
constexpr uint8_t SYNC_BYTES[4]  = {0x7F, 0xF0, 0x1C, 0xAF};

int main(int argc, char** argv)
{
    // read serial port path from command-line args (default to /dev/tty1, replace with /dev/pts/X for testing)
    const char* port = (argc > 1) ? argv[1] : "/dev/tty1";
    
    // call configure_uart(port) -> fd
    printf("Opening serial port: %d\n", port);
    int fd = configure_uart(port);
    if (fd < 0)
    {
        perror("UART open failed");
        return 1;
    }

    /*
        Goal for tonight: Implement a simple single-threaded loop that:

        1. Reads raw bytes from the UART port
        2. Buffers them
        3. Scans for the 0x7F F0 1C AF sync header
        4. Extracts a full 20-byte packet
        5. Passes it to parse_imu_packet()
        6. Prints the result to stdout

        Example output: 

        [UART] Found sync header at offset 12
        [IMU] Packet #42: X=10.00, Y=20.00, Z=40.00
        
        Future goal:

        1. Make loop run every 80ms to get raw data sent by python program
        2. Broadcast the data to localhost read by python program to verify data matches
        3. Probably needs to read chunks of data, not just 1 byte at a time
    
    */

    // Create a read buffer
    uint8_t buffer[UART_BUFFER_CAPACITY];

    // create a small sliding window for sync detection
    size_t buffer_size = 0; // buffer size is the current number of bytes in the buffer

    while (true)
    {
        // get a byte (can optimize by reading multiple bytes later)
        uint8_t byte;
        if (read(fd, &byte, 1) != 1) continue;

        // if the buffer is full
        if(buffer_size >= UART_BUFFER_CAPACITY)
        {
            // shift all the data left by 1 to make space
            memmove(&buffer[0], &buffer[1], UART_BUFFER_CAPACITY - 1);

            // update buffer size
            buffer_size = UART_BUFFER_CAPACITY - 1;
        }

        // append new byte to end of buffer
        buffer[buffer_size] = byte;
    
        // increment buffer_size
        buffer_size++;

        /* Scan for sync header until there's not enough bytes for a full packet */
        for (size_t i = 0; i + PACKET_SIZE <= buffer_size; ++i)
        {
            // Check if 4 bytes starting at that position match the sync header (0x7F F0 1C AF)
            if( memcmp(&buffer[i], SYNC_BYTES, 4) == 0)
            {
                // Call parse_imu_packet() on 20-byte window starting there
                ImuPacket pkt;
                if (parse_imu_packet(&buffer[i], PACKET_SIZE, pkt) == true)
                {
                    // print parsed result (later on it'll be sending over localhost probably)
                    printf("[IMU] Packet #%u: X=%.2f, Y=%.2f, Z=%.2f\n",
                        pkt.packet_count, pkt., pkt.y_rate, pkt.z_rate);

                    // Remove all bytes up through the end of that packet
                    size_t remaining = buffer_size - (i + PACKET_SIZE);

                    // shift buffer left
                    memmove(&buffer[0], &buffer[i + PACKET_SIZE], remaining);

                    // update buffer length
                    buffer_size = remaining;

                    break;
                }
            }
        }
    } 
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

    // Set VMIN and VTIME so read() returns after 1 byte or timeout (100ms? might cause problem later with 80ms loop)
    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 1;

    // Apply the settings
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("Failed to set UART config");
        return -1;
    }

    return fd;
}
