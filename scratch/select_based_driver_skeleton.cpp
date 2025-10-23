// Example: C++-style UART + UDP driver using Barr Group coding standards
// https://barrgroup.com/embedded-systems/how-to/c-coding-standard

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Constants
#define UART_BUFFER_SIZE        (256U)
#define IMU_PACKET_SIZE         (20U)
#define UDP_PORT                (9000U)
#define LOOP_INTERVAL_MS        (80U)

// Forward declarations
struct ImuPacket;
bool ParseImuPacket(const uint8_t* buffer, size_t len, ImuPacket& out);
void SendUdpPacket(int udp_fd, const struct sockaddr_in& dest_addr, const ImuPacket& pkt);

struct ImuPacket {
    uint32_t packet_count;
    float x_rate_rdps;
    float y_rate_rdps;
    float z_rate_rdps;
};

int OpenUart(const char* device)
{
    int fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        return -1;
    }

    struct termios tty;
    (void)memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) != 0) {
        close(fd);
        return -1;
    }

    (void)cfsetospeed(&tty, B921600);
    (void)cfsetispeed(&tty, B921600);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        close(fd);
        return -1;
    }

    return fd;
}

int SetupUdpSocket(struct sockaddr_in* p_dest_addr)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        return -1;
    }

    (void)fcntl(sockfd, F_SETFL, O_NONBLOCK);

    p_dest_addr->sin_family = AF_INET;
    p_dest_addr->sin_port = htons(UDP_PORT);
    (void)inet_pton(AF_INET, "127.0.0.1", &p_dest_addr->sin_addr);

    return sockfd;
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        (void)fprintf(stderr, "Usage: %s /dev/ttyUSB0\n", argv[0]);
        return EXIT_FAILURE;
    }

    int uart_fd = OpenUart(argv[1]);
    if (uart_fd < 0) {
        (void)perror("UART open failed");
        return EXIT_FAILURE;
    }

    struct sockaddr_in udp_addr;
    int udp_fd = SetupUdpSocket(&udp_addr);
    if (udp_fd < 0) {
        (void)perror("UDP socket setup failed");
        close(uart_fd);
        return EXIT_FAILURE;
    }

    uint8_t uart_buf[UART_BUFFER_SIZE] = {0};
    size_t uart_buf_pos = 0U;
    ImuPacket last_packet = {0U, 0.0F, 0.0F, 0.0F};

    while (true) {
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = LOOP_INTERVAL_MS * 1000U;

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(uart_fd, &read_fds);

        clock_t start = clock();

        int result = select(uart_fd + 1, &read_fds, NULL, NULL, &timeout);
        if ((result > 0) && FD_ISSET(uart_fd, &read_fds)) {
            ssize_t bytes = read(uart_fd, &uart_buf[uart_buf_pos], UART_BUFFER_SIZE - uart_buf_pos);
            if (bytes > 0) {
                uart_buf_pos += (size_t)bytes;

                for (size_t i = 0U; i + IMU_PACKET_SIZE <= uart_buf_pos; ++i) {
                    if ((uart_buf[i] == 0x7FU) &&
                        (uart_buf[i + 1U] == 0xF0U) &&
                        (uart_buf[i + 2U] == 0x1CU) &&
                        (uart_buf[i + 3U] == 0xAFU)) {

                        ImuPacket pkt;
                        if (ParseImuPacket(&uart_buf[i], IMU_PACKET_SIZE, pkt)) {
                            last_packet = pkt;
                        }
                    }
                }

                if (uart_buf_pos > (UART_BUFFER_SIZE - IMU_PACKET_SIZE)) {
                    uart_buf_pos = 0U;
                }
            }
        }

        SendUdpPacket(udp_fd, udp_addr, last_packet);

        clock_t end = clock();
        double elapsed_ms = (1000.0 * (double)(end - start)) / CLOCKS_PER_SEC;
        if (elapsed_ms < (double)LOOP_INTERVAL_MS) {
            (void)usleep((useconds_t)((LOOP_INTERVAL_MS - elapsed_ms) * 1000.0));
        }
    }

    // Never reached
    return EXIT_SUCCESS;
}
