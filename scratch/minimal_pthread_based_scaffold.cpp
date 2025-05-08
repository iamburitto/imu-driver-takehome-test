#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

// Define this struct somewhere accessible
typedef struct {
    uint32_t packet_count;
    float x_rate_rdps;
    float y_rate_rdps;
    float z_rate_rdps;
} imu_packet_t;

// Shared state
imu_packet_t latest_packet;
pthread_mutex_t latest_mutex = PTHREAD_MUTEX_INITIALIZER;

// UART parsing thread
void* uart_reader_loop(void* arg) {
    int fd = *(int*)arg;

    // LOOP:
    //   read 1 byte from fd
    //   add to buffer
    //   scan for sync
    //   if parse success:
    //     pthread_mutex_lock()
    //     copy to latest_packet
    //     pthread_mutex_unlock()

    return NULL;
}

// 80ms scheduler thread
void* broadcaster_loop(void* arg) {
    (void)arg;
    struct timespec interval = {0, 80 * 1000 * 1000}; // 80ms

    while (1) {
        nanosleep(&interval, NULL);

        pthread_mutex_lock(&latest_mutex);
        imu_packet_t pkt = latest_packet; // shallow copy
        pthread_mutex_unlock(&latest_mutex);

        printf("[BROADCAST] Packet #%u: X=%.2f Y=%.2f Z=%.2f\n",
            pkt.packet_count, pkt.x_rate_rdps, pkt.y_rate_rdps, pkt.z_rate_rdps);
    }

    return NULL;
}

int main(int argc, char** argv) {
    const char* port = (argc > 1) ? argv[1] : "/dev/tty1";
    int fd = configure_uart(port); // Your existing UART config
    if (fd < 0) return 1;

    pthread_t reader_thread, broadcaster_thread;

    pthread_create(&reader_thread, NULL, uart_reader_loop, &fd);
    pthread_create(&broadcaster_thread, NULL, broadcaster_loop, NULL);

    pthread_join(reader_thread, NULL);
    pthread_join(broadcaster_thread, NULL);

    return 0;
}
