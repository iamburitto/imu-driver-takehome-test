#include <sys/socket.h> //
#include <netinet/in.h> // 
#include <arpa/inet.h> //
#include <unistd.h> //

int sock_fd = socket(AF_INET, SOCK_DGRAM, 0); // creates a udp socket

sockaddr_in addr = {...};
// Set addr.sin_family = AF_INET
// Set addr.sin_port = htons(9000);
// Set addr.sin_addr.s_addr = inet_addr("127.0.0.1");

sockaddr_in dest_address{};
dest_address.sin_family = AF_INET;
dest_address.sin_port = htons(9000); // destination port
dest_address.sin_addr.s_addr = inet_addr("127.0.0.1"); // destination IP address (localhost)

// sendto(sock, buffer, len, 0, (struct sockaddr*)&addr, sizeof(addr));
// (optional) close(sock); when done