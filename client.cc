#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "err.h"

#define BUFFER_SIZE 1000
#define DEFAULT_PORT 20160


class Client {
private:
    int sock;

    struct addrinfo address_hints;
    struct addrinfo *address_result;

    struct sockaddr_in my_address;
    struct sockaddr_in server_address;

    char buffer[BUFFER_SIZE];

    size_t length;
    ssize_t send_length, receive_length;

    socklen_t receive_address_length;

public:
    Client(char *server_host, uint16_t server_port) {
        sock = socket(PF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            syserr("socket");
        }

        memset(&address_hints, 0, sizeof(struct addrinfo));
        address_hints.ai_family = AF_INET;
        address_hints.ai_socktype = SOCK_DGRAM;
        address_hints.ai_protocol = IPPROTO_UDP;
        address_hints.ai_flags = 0;
        address_hints.ai_addrlen = 0;
        address_hints.ai_addr = NULL;
        address_hints.ai_canonname = NULL;
        address_hints.ai_next = NULL;
        if (getaddrinfo(server_host, NULL, &address_hints, &address_result) != 0) {
            syserr("getaddrinfo");
        }

        my_address.sin_family = AF_INET; // IPv4
        my_address.sin_addr.s_addr = ((struct sockaddr_in*)address_result->ai_addr)->sin_addr.s_addr;
        my_address.sin_port = htons(server_port);

        freeaddrinfo(address_result);
    }

    ~Client() {
        if (close(sock) == -1) {
            syserr("close");
        }
        printf("Deconstructing client.\n");
    }

    void send_datagram(char character) {
        buffer[0] = character;
        buffer[1] = ' ';
        buffer[2] = 'l';
        buffer[3] = 'o';
        buffer[4] = 'l';
        length = 5;

        receive_address_length = (socklen_t)sizeof(my_address);
        send_length = sendto(sock, buffer, length, 0, (struct sockaddr *)&my_address, receive_address_length);
        if (send_length != (ssize_t)length) {
            syserr("partial / failed sending datagram to server");
        }
    }

    void read_datagrams() {
        memset(buffer, 0, sizeof(buffer));
        length = (size_t)sizeof(buffer) - 1;

        receive_address_length = (socklen_t)sizeof(server_address);
        receive_length = recvfrom(sock, buffer, length, 0, (struct sockaddr *)&server_address, &receive_address_length);
        if (receive_length < 0) {
            syserr("read");
        }
        printf("%s", buffer);
    }

};

void print_usage(char *filename) {
    printf("Usage: %s timestamp character server_host [server_port]\n", filename);
}

int main(int argc, char *argv[]) {
    uint16_t port = DEFAULT_PORT;

    if (argc < 4 || argc > 5) {
        print_usage(argv[0]);
        return 1;
    }
    // TODO: check if port is correct, fuck this shit for now
    if (argc == 5) {
        port = (uint16_t)atoi(argv[4]);
    }

    if (strlen(argv[2]) > 1) {
        print_usage(argv[0]);
        printf("ERROR: Invalid size of character parameter ( > 1).\n");
        return 1;
    }

    Client *c = new Client(argv[3], port);
    c->send_datagram(argv[2][0]);

    while (true)
        c->read_datagrams();
}
