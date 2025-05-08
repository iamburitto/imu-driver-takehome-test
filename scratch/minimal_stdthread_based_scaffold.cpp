#include <thread>
#include <mutex>
#include <chrono>
#include <cstdio>

// Shared data
ImuPacket g_latest_packet;
std::mutex g_latest_mutex;

void uart_reader_thread(int fd) {
    // LOOP:
    //   read 1 byte at a time from fd
    //   append to buffer
    //   if sync + 20 bytes found:
    //       call parse_imu_packet()
    //       if success:
    //           lock mutex
    //           latest_packet = parsed
    //           unlock mutex
}

void broadcaster_loop() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(80));

        // lock mutex
        // copy latest_packet to local var
        // unlock mutex

        // print or send packet (for now, just print)
        // printf("BROADCAST Packet #%u: X=..., Y=..., Z=...\n");
    }
}

int main(int argc, char** argv) {
    const char* port = (argc > 1) ? argv[1] : "/dev/tty1";
    int fd = configure_uart(port);
    if (fd < 0) return 1;

    std::thread reader(uart_reader_loop, fd);
    std::thread broadcaster(broadcaster_loop);

    reader.join();
    broadcaster.join();

    return 0;
}
