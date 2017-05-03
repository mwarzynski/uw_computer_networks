#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <queue>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <limits.h>
#include <poll.h>
#include <fcntl.h>

#include "datagram.h"
#include "parse.h"

#define MAX_DATAGRAMS 4096
#define MAX_SOCKETS 42


class Buffer {

    std::queue<datagram> data;

public:

    void insert(datagram d) {
        if (data.size() == MAX_DATAGRAMS)
            data.pop();
        data.push(d);
    }

    bool pop(datagram *d) {
        if (data.empty())
            return false;
        d = &data.front();
        data.pop();
        return true;
    }
};


class Server {

    Buffer *buffer;

    bool finish = false;
    unsigned int active_clients = 0;

    struct sockaddr_in server;
    struct pollfd client[MAX_SOCKETS];

public:

    Server() {
        buffer = new Buffer();

        for (int i = 0; i < MAX_SOCKETS; ++i) {
            client[i].fd = -1;
            client[i].events = POLLIN;
        }

        client[0].fd = socket(PF_INET, SOCK_STREAM, 0);
        if (client[0].fd < 0) {
            perror("Opening stream socket");
            exit(EXIT_FAILURE);
        }

        server.sin_family = AF_INET;
        server.sin_addr.s_addr = htonl(INADDR_ANY);
        server.sin_port = 0;
        if (bind(client[0].fd, (struct sockaddr*)&server, (socklen_t)sizeof(server)) < 0) {
            perror("Binding stream socket");
            exit(EXIT_FAILURE);
        }

        ssize_t length = sizeof(server);
        if (getsockname (client[0].fd, (struct sockaddr*)&server, (socklen_t*)&length) < 0) {
            perror("Getting socket name");
            exit(EXIT_FAILURE);
        }

        if (listen(client[0].fd, 5) == -1) {
            perror("Starting to listen");
            exit(EXIT_FAILURE);
        }
    }

    ~Server() {

    }

    void run() {
        int ret;
        do {
            for (int i = 0; i < MAX_SOCKETS; ++i)
                client[i].revents = 0;

            ret = poll(client, MAX_SOCKETS, 5000);
            if (ret < 0) {
                perror("poll");
                continue;
            } else if (ret == 0) {
                continue;
            }

            if (client[0].revents & POLLIN)
                get_new_connection();

            for (int i = 1; i < MAX_SOCKETS; ++i) {
                if (client[i].fd != -1 && (client[i].revents & (POLLIN | POLLERR))) {
                    /* Czytam od klienta. */
                    rval = read(client[i].fd, buf[i], BUF_SIZE);
                    if (rval < 0) {
                        perror("Reading stream message");
                        if (close(client[i].fd) < 0)
                            perror("close");
                        client[i].fd = -1;
                        active_clients -= 1;
                    }
                    else if (rval == 0) {
                        fprintf(stderr, "Ending connection (%d)\n", i);
                        if (close(client[i].fd) < 0)
                            perror("close");
                        client[i].fd = -1;
                        active_clients -= 1;
                    }
                    else {
                        fprintf(stderr, "Received %zd bytes (connection %d)\n", rval, i);
                        buf_len[i] = rval;
                        buf_pos[i] = 0;
                        client[i].events = POLLOUT; /* Teraz chcę pisać, a już nie czytać. */
                    }
                }

                if (client[i].fd != -1 && (client[i].revents & POLLOUT)) {
                    /* Piszę do klienta. */
                    rval = write(client[i].fd, buf[i] + buf_pos[i], buf_len[i] - buf_pos[i]);
                    if (rval < 0) {
                        perror("write");
                        if (close(client[i].fd) < 0)
                            perror("close");
                        client[i].fd = -1;
                        active_clients -= 1;
                    }
                    else {
                        fprintf(stderr, "Sent %zd bytes (connection %d)\n", rval, i);
                        buf_pos[i] += rval;
                        if (buf_pos[i] == buf_len[i])
                            client[i].events = POLLIN; /* Przełączam się na czytanie. */

                    }
                }
            }

        } while (!finish || active_clients > 0);
    }


private:

    void get_new_connection() {
        int socket = accept(client[0].fd, (struct sockaddr*)0, (socklen_t*)0);
        if (socket == -1)
            perror("accept");
        else {
            int i;
            if (fcntl(socket, F_SETFL, O_NONBLOCK) < 0)
                perror("fcntl");
            for (i = 1; i < MAX_SOCKETS; ++i) {
                if (client[i].fd == -1) {
                    fprintf(stderr, "Received new connection (%d)\n", i);
                    client[i].fd = socket;
                    client[i].events = POLLIN;
                    active_clients += 1;
                    break;
                }
            }
            if (i >= MAX_SOCKETS) {
                fprintf(stderr, "Too many clients\n");
                if (close(socket) < 0)
                    perror("close");
            }
        }
    }

};


void print_usage(char *command) {
    printf("Usage: %s port filename\n", command);
}

void validate_arguments(int argc, char *argv[]) {
    if (argc != 3) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    uint16_t port = parse_port(argv[1]);
    if (port == 0) {
        printf("Port must be within range 1 - 65535.\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (!file_exists(argv[2])) {
        printf("Given filename is invalid. Make sure that provided file exists.\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    validate_arguments(argc, argv);

    Server s = Server();
    s.run();

    return EXIT_SUCCESS;
}
