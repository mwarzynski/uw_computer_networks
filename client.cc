#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>

#include "err.h"
#include "datagram.h"
#include "parse.h"

#define BUFFER_SIZE 66000
#define DEFAULT_SERVER_PORT 20160

class Client {

    int sock;

    struct addrinfo address_hints;
    struct addrinfo *address_result;

    struct sockaddr_in my_address;
    struct sockaddr_in server_address;

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

        my_address.sin_family = AF_INET;
        my_address.sin_addr.s_addr = ((struct sockaddr_in*)address_result->ai_addr)->sin_addr.s_addr;
        my_address.sin_port = htons(server_port);

        freeaddrinfo(address_result);
    }

    ~Client() {
        if (close(sock) == -1) {
            syserr("close");
        }
    }

    void send_datagram(char character) {
        time_t raw_time;
        time(&raw_time);

        datagram d;
        d.timestamp = (uint64_t)raw_time;
        d.character = character;

        size_t length = sizeof(d);

        receive_address_length = (socklen_t)sizeof(my_address);
        send_length = sendto(sock, &d, length, 0, (struct sockaddr *)&my_address, receive_address_length);
        if (send_length != (ssize_t)length)
            syserr("partial / failed sending datagram to server");
    }

    void read_datagram() {
        datagram d;
        receive_address_length = (socklen_t)sizeof(server_address);
        receive_length = recvfrom(sock, &d, sizeof(d), 0, (struct sockaddr *)&server_address, &receive_address_length);
        if (receive_length < 0)
            syserr("reading datagram from server err");

        if (!parse_datagram(&d))
            printf("INVALID DATAGRAM: %ld%c%s\n", d.timestamp, d.character, d.text);
        else
            printf("%ld%c%s\n", d.timestamp, d.character, d.text);
    }
};

void print_usage(char *filename) {
    printf("Usage: %s timestamp character server_host [server_port]\n", filename);
}

int main(int argc, char *argv[]) {
    uint16_t port = DEFAULT_SERVER_PORT;

    if (argc < 4 || argc > 5) {
        print_usage(argv[0]);
        return 1;
    }
    if (argc == 5) {
        port = parse_port(argv[4]);
        if (port == 0) {
            printf("Port must be within range 1 - 65535.\n");
            return 1;
        }
    }

    if (strlen(argv[2]) > 1) {
        print_usage(argv[0]);
        printf("ERROR: Invalid size of character parameter ( > 1).\n");
        return 1;
    }

    Client *c = new Client(argv[3], port);
    c->send_datagram(argv[2][0]);

    while (true)
        c->read_datagram();
}
