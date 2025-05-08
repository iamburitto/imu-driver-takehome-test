
#include <pthread.h>
#include <unistd.h> // POSIX standard constants
#include <stdio.h>
#include <fcntl.h> // for file control operstions like open()
#include <termios.h>
#include <time.h> // for nanosleep()
#include <errno.h>
#include <stdint.h> // for fixed-width integer types

// maybe I don't need these c++ wrapper ones?
#include <cstdint> 
#include <cstring>
#include <cstdlib>

#include "imu_parser.hpp" // includes my fendian.hpp to deal with endianness for floats

        
// TODO: put these somewhere better later
constexpr size_t UART_BUFFER_CAPACITY = 1024; // TODO: calculate how big it actually needs to be at this baud rate for 20 byte packets and running every 80ms
constexpr size_t PACKET_SIZE  = 20;
constexpr uint8_t SYNC_BYTES[4]  = {0x7F, 0xF0, 0x1C, 0xAF};

int configure_uart(const char* device);

/*
    Right now this main() is a simple single-threaded loop that:

    1. Reads raw bytes from the UART port
    2. Buffers them
    3. Scans for the 0x7F F0 1C AF sync header
    4. Extracts a full 20-byte packet
    5. Passes it to parse_imu_packet()
    6. Prints the result to stdout

    Example output: 

    [UART] Found sync header at offset 12
    [IMU] Packet #42: X=10.00, Y=20.00, Z=40.00
    
    Next:

    1. Make loop run every 80ms to get raw data sent by python program
    2. Broadcast the data to localhost read by python program to verify data matches
    3. Probably needs to read chunks of data later, not just 1 byte at a time
*/

int main(int argc, char** argv)
{
    // read serial port path from command-line args (default to /dev/tty1, replace with /dev/pts/X for testing)
    const char* port = (argc > 1) ? argv[1] : "/dev/tty1";
    
    // call configure_uart(port) -> fd
    printf("Opening serial port: %s\n", port);
    int fd = configure_uart(port);
    if (fd < 0)
    {
        perror("UART serial port open failed");
        return 1;
    }

    // Create a read buffer
    uint8_t buffer[UART_BUFFER_CAPACITY];

    // create a small sliding window for sync detection
    size_t buffer_size_nbytes = 0; // buffer size is the current number of bytes in the buffer

    // honestly if I'm going to do 1 byte at a time I should probably be using a circular buffer, the logic would be easier.
    // oh well.

    while (true) // scroll to the bottom to see a comment with possible optimizations
    {
        // get a byte (can optimize by reading multiple bytes later)
        uint8_t byte;
        if (read(fd, &byte, 1) != 1) continue;

        // if the buffer is full
        if(buffer_size_nbytes >= UART_BUFFER_CAPACITY)
        {
            // shift all the data left by 1 to drop the oldest byte and make space
            // TODO: print some kind of debug info here so we know we're dropping one?
            memmove(&buffer[0], &buffer[1], UART_BUFFER_CAPACITY - 1);

            // update buffer size to make room
            buffer_size_nbytes = UART_BUFFER_CAPACITY - 1;
        }

        // append new byte to end of buffer
        buffer[buffer_size_nbytes] = byte;
    
        // increment buffer_size
        buffer_size_nbytes++;

        // NOTE: This whole thing would be a lot easier for me to read personally if I put it in a state machine,
        // but the goal right now is functionality not prettiness, and some folks don't like state machines as much as I do.

        // Slide 20 byte window right to scan for SYNC 
        for (size_t i = 0; i + PACKET_SIZE <= buffer_size_nbytes; ++i)
        {
            // Check if 4 bytes starting at that position match the sync header (0x7F F0 1C AF)
            if( memcmp(&buffer[i], SYNC_BYTES, 4) == 0)
            {
                // Call parse_imu_packet() on 20-byte window starting there
                ImuPacket pkt;
                
                if (parse_imu_packet(&buffer[i], PACKET_SIZE, pkt) == true)
                {
                    printf("[UART] Found sync header at offset %zd\n", i);
                    printf("[IMU] Packet #%u: X=%.2f, Y=%.2f, Z=%.2f\n",
                        pkt.packet_count, pkt.x_rate_rdps, pkt.y_rate_rdps, pkt.z_rate_rdps);

                    // Remove all bytes up through the end of that packet
                    size_t remaining = buffer_size_nbytes - (i + PACKET_SIZE);

                    // shift buffer left to get rid of the packet (later we can optimize to only shift when 20 byte window hits the end)
                    memmove(&buffer[0], &buffer[i + PACKET_SIZE], remaining);

                    // update buffer length
                    buffer_size_nbytes = remaining;

                    break;
                }
            }
        }
    } 

    /*
        Visualizing the above and possible optimizations:
                
        Buffer is a scrolling strip of tape:
        You read in bytes on the right.

        Right now, I  scan from the left for a 4-byte sync window.
        When we parse a packet, we cut it out, and slide the remaining tape left.

        Optimizations:

        If we don't wanna scoot the data that much we can
        just skip the cut and paste until we get close to the end.
        Need a start_of_window pointer for that.

        Since we're only doing one byte at a time right now:
        A simple circular buffer and/or a state machine would do the trick too.

        Either way, we could also change this code to process as many bytes as possible instead of just 1.

        It could improve performance and possibly jitter to have fewer system calls.
        And its often necessary with real hardware:
        - small buffers in FPGA or hardware UART implementation
        - bursty data
        - lack of DMA FIFOs

        But I'd have to set up the UART differently with the VTIME business cause its only 1 byte right now.
        And until I know how many samples this thing is actually going to be sending out in real life, its easier to wait.
        
        */
}

/*
    just to make sure I could get bytes from the emulator side with socat
*/
int main_initial_test_version(int argc, char** argv) {
    const char* port = "/dev/tty1";  // default - replace with /dev/pts/X for testing
    
    if (argc > 1)
    {
        port = argv[1]; // override port with command line argument
    }

    printf("Opening serial port: %s\n", port);
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
    // TODO: read man termios to actually understand these options that I got from stackoverflow
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

    // Set VMIN and VTIME so read() returns after 1 byte or timeout (100ms timeout default I Think? might cause problem later with 80ms loop)
    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 1;

    // Apply the settings
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("Failed to set UART config");
        return -1;
    }

    return fd;
}
