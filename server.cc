#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "err.h"

#define BUFFER_SIZE   1000
#define PORT_NUM     10001

int main(int argc, char *argv[]) {
    int sock;
    int flags, sflags;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    char buffer[BUFFER_SIZE];
    socklen_t sender_address_length, receiver_address_length;
    ssize_t len, snd_len;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        syserr("socket");

    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
    server_address.sin_port = htons(PORT_NUM); // default port for receiving is PORT_NUM

    if (bind(sock, (struct sockaddr *)&server_address, (socklen_t)sizeof(server_address)) < 0)
        syserr("bind");

    sender_address_length = (socklen_t)sizeof(client_address);
    for (;;) {
        do {

            receiver_address_length = (socklen_t)sizeof(client_address);
            flags = 0;

            len = recvfrom(sock, buffer, sizeof(buffer), flags, (struct sockaddr *)&client_address, &receiver_address_length);
            if (len < 0) {
                syserr("error on datagram from client socket");
            } else {
                printf("read from socket: %zd bytes: %.*s\n", len, (int) len, buffer);
                sflags = 0;
                snd_len = sendto(sock, buffer, (size_t) len, sflags, (struct sockaddr *)&client_address, sender_address_length);
                if (snd_len != len) {
                    syserr("error on sending datagram to client socket");
                }
            }
        } while (len > 0);

        printf("finished exchange\n");
    }
}
