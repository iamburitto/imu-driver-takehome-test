
// maybe I don't need these c++ wrapper ones?
#include <cstdint> 
#include <cstring>
#include <cstdlib>

/* for std::thread */
#include <thread>
#include <mutex>
#include <chrono>
#include <cstdio>

#include <unistd.h> // POSIX standard constants
#include <stdio.h>
#include <string.h>
#include <stdint.h> // for fixed-width integer types
#include <time.h> // for nanosleep()

#include <fcntl.h> // for file control operstions like open()
#include <termios.h>
#include <errno.h>

#include "uart.hpp"
#include "imu_parser.hpp" // includes my fendian.hpp to deal with endianness for floats

// global shared packet + mutex (do these need to be volatile?)
imu_packet_t g_latest_packet;
std::mutex g_latest_mutex;

// IMU packet parsing constants
constexpr size_t UART_BUFFER_CAPACITY = 1024; // TODO: calculate how big it actually needs to be at this baud rate for 20 byte packets and running every 80ms
constexpr size_t PACKET_SIZE  = 20;
constexpr uint8_t SYNC_BYTES[4]  = {0x7F, 0xF0, 0x1C, 0xAF};

void uart_reader_thread(int fd) // scroll to the end of this file for a list of possible optimizations
{
    /*
        1. Buffers raw bytes from the UART port 1 at a time (shifts left to drop oldest byte if full)
        2. Scans for the 0x7F F0 1C AF sync header (sliding 20 byte window)
        3. Calls parse_imu_packet() to extract packet fields to imu_packet_t struct
        4. On successful extraction: lock mutex, update g_latest_packet, unlock mutex
    */

    uint8_t buffer[UART_BUFFER_CAPACITY];
    size_t buffer_size_nbytes = 0;

    while (true)
    {
        uint8_t byte;
        if (read(fd, &byte, 1) != 1) continue;

        if(buffer_size_nbytes >= UART_BUFFER_CAPACITY)
        {
            // shift all the data left by 1 to drop the oldest byte and make space
            memmove(&buffer[0], &buffer[1], UART_BUFFER_CAPACITY - 1);
            buffer_size_nbytes = UART_BUFFER_CAPACITY - 1;
        }

        // append new byte to end of buffer
        buffer[buffer_size_nbytes] = byte;
        buffer_size_nbytes++;

        // NOTE: This whole thing would be a lot easier for me to read personally if I put it in a state machine,
        // but the goal right now is functionality not prettiness, and some folks don't like state machines as much as I do.

        // scan for SYNC word at start of 20 byte window 
        for (size_t i = 0; i + PACKET_SIZE <= buffer_size_nbytes; ++i)
        {
            // Check if 4 bytes starting at that position match the sync header (0x7F F0 1C AF)
            if( memcmp(&buffer[i], SYNC_BYTES, 4) == 0)
            {
                imu_packet_t pkt;
                if (parse_imu_packet(&buffer[i], PACKET_SIZE, pkt) == true)
                {
                    g_latest_mutex.lock();
                    g_latest_packet = pkt; // shallow copy is ok here

                    printf("[UART] Found sync header at offset %zd\n", i);
                    printf("[IMU] Packet #%u: X=%.2f, Y=%.2f, Z=%.2f\n",
                        pkt.packet_count, pkt.x_rate_rdps, pkt.y_rate_rdps, pkt.z_rate_rdps);

                    g_latest_mutex.unlock();

                    // Remove all bytes up through the end of that packet
                    size_t remaining = buffer_size_nbytes - (i + PACKET_SIZE);

                    // shift buffer left to get rid of the packet
                    memmove(&buffer[0], &buffer[i + PACKET_SIZE], remaining);

                    // update buffer length
                    buffer_size_nbytes = remaining;

                    break;
                }
            }
        }
    } 
}

void broadcaster_80ms_thread()
{
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(80));

        g_latest_mutex.lock();
        imu_packet_t pkt = g_latest_packet; // shallow copy ok here
        g_latest_mutex.unlock();

        // send this to localhost later
        printf("[BROADCAST] Packet #%u: X=%.2f Y=%.2f Z=%.2f\n",
            pkt.packet_count, pkt.x_rate_rdps, pkt.y_rate_rdps, pkt.z_rate_rdps);
    }
}

int main(int argc, char** argv)
{
    const char* port = (argc > 1) ? argv[1] : "/dev/tty1"; // pass in /dev/pts/X for testing
    int fd = configure_uart(port);
    if (fd < 0) return 1;

    // Might be better to use pthreads directly for real flight code (more common/more control over time) - might make a new version after this.

    std::thread reader(uart_reader_thread, fd);
    std::thread broadcaster(broadcaster_80ms_thread);

    reader.join();
    broadcaster.join();

    return 0;
}

/*
    EVERYTHING AFTER THIS IS JUST PREVIOUS VERSIONS OF MAIN LOOP WHILE I WAS GETTING THE CODE WORKING...
    PLUS SOME OPTIMIZATION IDEAS THAT SHOULD BE MOVED TO THE README LATER BUT HERE'S A NOTE IF I FORGET
*/

int main_second_version_that_parses_and_extracts_packets(int argc, char** argv)
{
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
                imu_packet_t pkt;
                
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

}

int main_initial_uart_test_version(int argc, char** argv)
{
    /*
        just to make sure I could get bytes from the emulator side with socat
    */

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

/*
        Visualizing the above and possible optimizations:
                
        Buffer is a scrolling strip of tape:
        You read in bytes on the right.

        Right now, I  scan from the left for a 4-byte sync window.
        When we parse a packet, we cut it out, and slide the remaining tape left.

        Optimizations:

        only shift data left when we run out of room, not every time
        (i.e. skip the cut and paste until we get close to the end.)
        Need a start_of_window pointer for that.

        Since we're only doing one byte at a time right now:
        A simple circular buffer and/or a state machine would do the trick too.

        Either way, we could also change this code to process as many bytes as possible instead of just 1.

        Doing that could improve performance and possibly jitter to have fewer system calls.
        Also its often necessary with real hardware because:
        - small buffers in FPGA or hardware UART implementation
        - bursty data and/or lots of data at once
        - lack of DMA FIFOs

        I'd have to set up the UART differently with the VTIME business cause its only 1 byte right now.
        Until I know how many samples this thing is actually going to be sending out in real life, its easier to wait.
        
*/