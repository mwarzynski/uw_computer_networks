#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "err.h"

#define BUFFER_SIZE 1000

void send_datagram() {

}

void receive_datagram() {

}


int main(int argc, char *argv[]) {
    int sock;
    struct addrinfo address_hints;
    struct addrinfo *address_result;

    int i, flags, sflags;
    char buffer[BUFFER_SIZE];
    size_t length;
    ssize_t send_length, receive_length;
    struct sockaddr_in my_address;
    struct sockaddr_in server_address;
    socklen_t receive_address_length;

    if (argc < 3 || argc > 4) {
        printf("Usage: %s timestamp c host [port]\n", argv[0]);
        return 1;
    }

    memset(&address_hints, 0, sizeof(struct addrinfo));
    address_hints.ai_family = AF_INET; // IPv4
    address_hints.ai_socktype = SOCK_DGRAM;
    address_hints.ai_protocol = IPPROTO_UDP;
    address_hints.ai_flags = 0;
    address_hints.ai_addrlen = 0;
    address_hints.ai_addr = NULL;
    address_hints.ai_canonname = NULL;
    address_hints.ai_next = NULL;
    if (getaddrinfo(argv[1], NULL, &address_hints, &address_result) != 0) {
        syserr("getaddrinfo");
    }

    my_address.sin_family = AF_INET; // IPv4
    my_address.sin_addr.s_addr = ((struct sockaddr_in*)address_result->ai_addr)->sin_addr.s_addr;
    my_address.sin_port = htons((uint16_t)atoi(argv[2]));

    freeaddrinfo(address_result);

    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        syserr("socket");
    }

    for (i = 3; i < argc; i++) {
        length = strnlen(argv[i], BUFFER_SIZE);
        if (length == BUFFER_SIZE) {
            fprintf(stderr, "ignoring long parameter %d\n", i);
            continue;
        }

        printf("sending to socket: %s\n", argv[i]);
        sflags = 0;
        receive_address_length = (socklen_t)sizeof(my_address);
        send_length = sendto(sock, argv[i], length, sflags, (struct sockaddr *)&my_address, receive_address_length);
        if (send_length != (ssize_t)length) {
            syserr("partial / failed write");
        }

        (void) memset(buffer, 0, sizeof(buffer));
        flags = 0;
        length = (size_t)sizeof(buffer) - 1;
        receive_address_length = (socklen_t)sizeof(server_address);
        receive_length = recvfrom(sock, buffer, length, flags, (struct sockaddr *)&server_address, &receive_address_length);

        if (receive_length < 0) {
            syserr("read");
        }
        printf("read from socket: %zd bytes: %s\n", receive_length, buffer);
    }

    if (close(sock) == -1) {
        syserr("close");
    }

    return 0;
}
