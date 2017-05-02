#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>

#include "datagram.h"
#include "parse.h"

#define BUFFER_SIZE 66000
#define DEFAULT_SERVER_PORT 20160

class Client {

    int sock;

    sockaddr_in send_address;
    sockaddr_in receive_address;

public:
    Client(sockaddr_in server_address) {
        sock = socket(PF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            fprintf(stderr, "Socket error.\n");
            exit(EXIT_FAILURE);
        }

        this->send_address = server_address;
    }

    ~Client() {
        if (close(sock) == -1)
            fprintf(stderr, "Closing socket error.\n");
    }

    void send_datagram(char character) {
        datagram d;
        prepare_datagram(&d, character, "");

        size_t length = sizeof(d);
        if (send(sock, send_address, &d, length) != (ssize_t)length)
            err_with_ip("Sending datagram error, destination=", send_address);
    }

    void read_datagram() {
        datagram d;

        if (receive(sock, receive_address, &d, sizeof(d)) < 0) {
            fprintf(stderr, "Reading datagram from server error.\n");
            return;
        }

        if (!parse_datagram(&d)) {
            err_with_ip("Parsing datagram error from=", receive_address);
            return;
        }

        print_datagram(&d);
    }
};

void print_usage(char *filename) {
    printf("Usage: %s timestamp character server_host [server_port]\n", filename);
}

void parse_arguments(int argc, char *argv[], sockaddr_in *dest, uint16_t *port) {
    if (argc < 4 || argc > 5) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    if (argc == 5) {
        *port = parse_port(argv[4]);
        if (*port == 0) {
            printf("Port must be within range 1 - 65535.\n");
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    if (strlen(argv[2]) > 1) {
        printf("ERROR: Invalid size of character parameter ( > 1).\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    if (strlen(argv[2]) != 1 || !isalpha(argv[2][0])) {
        printf("ERROR: Invalid character (isn't alpha numeric or longer than 1 character).\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    if (!parse_host(argv[3], dest)) {
        printf("ERROR: Invalid server_host.\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    sockaddr_in destination;
    uint16_t port = DEFAULT_SERVER_PORT;

    parse_arguments(argc, argv, &destination, &port);
    destination.sin_port = htons(port);

    Client *c = new Client(destination);
    c->send_datagram(argv[2][0]);

    while (true)
        c->read_datagram();
}
