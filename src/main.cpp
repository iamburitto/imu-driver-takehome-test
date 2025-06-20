
// maybe I don't need some of these c++ wrapper ones, gotta clean it up
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
#include <time.h> // for nanosleep(), can drop if I don't switch over to regular p_threads

#include <fcntl.h> // for file control operstions like open()
#include <termios.h>
#include <errno.h>

/* socket stuff */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

/* my own headers */
#include "uart.hpp"
#include "imu_parser.hpp" // includes my fendian.hpp to deal with endianness for floats

// global shared packet + mutex (do these need to be volatile? I don't think so since no ISR or hardware registers)
imu_packet_t g_latest_packet;
std::mutex g_latest_mutex;

// IMU packet parsing constants
constexpr size_t UART_BUFFER_CAPACITY = 1024; // TODO: calculate how big it actually needs to be at this baud rate for 20 byte packets and running every 80ms
constexpr size_t PACKET_SIZE  = 20;
constexpr uint8_t SYNC_BYTES[4]  = {0x7F, 0xF0, 0x1C, 0xAF};

#define MAX_UDP_PAYLOAD_SIZE 128

/*
    TODO: To make the C++ side as robust as the Python side:
    
    1. Use select() or poll():
        - For both UART (/dev/tty*) and the UDP socket.
        - Prevents blocking issues and enables precise timing logic.
        
    2. Make file descriptors non-blocking:
        - Use fcntl(fd, F_SETFL, O_NONBLOCK) for the UART.
        - Set MSG_DONTWAIT or non-blocking flags for the UDP socket.
        
    3. Control execution rate with explicit time tracking:
        - Measure elapsed loop time using start_time = current_time and subtracting that from 80ms to sleep() for the exact remainder → makes the loop run close to real-time 50Hz.
        
    4. Optionally log dropped/skipped packets (if loop takes too long) to assess timing margins.

*/

void uart_reader_thread(int fd)
{
    /*
        1. Buffers raw bytes from the UART port 1 at a time (shifts left to drop oldest byte if full)
        2. Scans for the 0x7F F0 1C AF sync header (sliding 20 byte window)
        3. Calls parse_imu_packet() to extract packet fields to imu_packet_t struct
        4. On successful extraction: lock mutex, update g_latest_packet, unlock mutex
    */
    
    /*
        Visualizing:
                
        - Buffer is a scrolling strip of tape:
        - Scan from the left for a 4-byte sync at the beginning of a 20 byte window
        - When we parse a packet, we cut out the 20 byte window, and slide the remaining tape all the way to the left.

        Optimizations:

        1. instead of always using memmove() to shift the data left, use pointers to move the 20 byte window through the data to the right.
          - memmove() to shift leftover bytes and 20 byte window to the beginning of the buffer when we are close to running out of room
            - NOTE: This should only happen occasionally
          - can use read_index and write_index pointers and treat it like a circular buffer that just isn't allowed to wrap
            - NOTE: shifting left is the same algorithm as defragmenting a real circular buffer
          
          Why:
          
            - Shifting data with memmove() is expensive if there's a lot of it
            - It would better support optimization 2 below:

        2. update to use select() to read as many bytes as available instead of just 1 at a time (nonblocking)

            Why:
            
            - fewer system calls could improve performance and possibly jitter
            - often necessary with real hardware to avoid missing data because:
              - small buffers in FPGA or hardware UART implementation
              - bursty data and/or lots of data at once
              - lack of DMA FIFOs
    
            How:
            
            - I'd have to set up the UART differently with the VTIME business cause its only 1 byte right now.
            - Until I know how many samples this thing is actually going to be sending out in real life, its easier to wait.
        
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
                    g_latest_mutex.lock(); // possible improvement if I use std::lock_guard<std::mutex> lock(g_latest_mutex); guards against early returns/exceptions and unlocks automagically says the internet
                    g_latest_packet = pkt; // shallow copy is ok here

                    // printf("[READER][UART] Found sync header at offset %zd\n", i);
                    printf("[DRIVER][UART_READ]: Packet #%u: X=%.2f, Y=%.2f, Z=%.2f\n",
                        pkt.packet_count, pkt.x_rate_rdps, pkt.y_rate_rdps, pkt.z_rate_rdps);

                    g_latest_mutex.unlock();

                    // Remove all bytes up through the end of that packet
                    size_t remaining = buffer_size_nbytes - (i + PACKET_SIZE);

                    // shift all remaining bytes in buffer left to get rid of the packet
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
    // open up a UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return;
    }
    
    // set up socket destination address (broadcast to localhost, python script emulator/tester will be reading it)
    sockaddr_in dest_address{};
    dest_address.sin_family = AF_INET;
    dest_address.sin_port = htons(9000); // destination port
    dest_address.sin_addr.s_addr = inet_addr("127.0.0.1"); // destination IP address (localhost)
    
    static imu_packet_t last_pkt;

    while(true)
    {
        // I want to switch to using regular non-c++ pthreads for actual flight code, cause this isn't enough control over the time I don't think and I hate all these namespaces ::
        std::this_thread::sleep_for(std::chrono::milliseconds(80));

        g_latest_mutex.lock(); // std::lock_guard<std::mutex> lock(g_latest_mutex); guards against early returns/exceptions and unlocks automagically says the internet
        imu_packet_t pkt = g_latest_packet; // shallow copy ok here
        g_latest_mutex.unlock();
        
        // check for stale
        if(memcmp(&last_pkt, &pkt, sizeof(imu_packet_t)) == 0)
        {
            continue;
        }
        
        // check for nothing actually received yet (actual packet count from emulator will start at 1)
        if(pkt.packet_count <= 0)
        {
            continue;
        }
        
        last_pkt = pkt; // store the last one for future stale check
        
        // Format packet into a text buffer
        char udp_payload_buf[MAX_UDP_PAYLOAD_SIZE] = {'\0'};
        int bytes_written = snprintf(udp_payload_buf, sizeof(udp_payload_buf),
            "Packet #%u: X=%.2f Y=%.2f Z=%.2f",
            pkt.packet_count,
            pkt.x_rate_rdps,
            pkt.y_rate_rdps,
            pkt.z_rate_rdps);
        
        if (bytes_written >= MAX_UDP_PAYLOAD_SIZE)
        {
            perror("UDP payload was truncated.");
        }
        else
        {
            printf("[DRIVER][UDP_WRITE]: %s\n",udp_payload_buf);
            printf("Bytes written: %d\n\n", bytes_written);
        
            // write to localhost:9000
            sendto(sock, udp_payload_buf, strlen(udp_payload_buf), 0,
                (struct sockaddr*)&dest_address, sizeof(dest_address));
        }
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