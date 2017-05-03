#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <queue>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <poll.h>
#include <map>
#include <fstream>

#include "datagram.h"
#include "parse.h"

#define MAX_DATAGRAMS 4096
#define MAX_SOCKETS 42


class Receive {

    std::queue<std::pair<datagram, sockaddr_in>> buffer;

public:

    bool empty() {
        return buffer.empty();
    }

    std::pair<datagram, sockaddr_in> pop() {
        std::pair<datagram, sockaddr_in> ret = buffer.front();
        buffer.pop();
        return ret;
    }

    void add(sockaddr_in client, datagram d) {
        if (buffer.size() == MAX_DATAGRAMS)
            buffer.pop();
        buffer.push(std::make_pair(d, client));
    }
};

class Responds {

    std::string file_content;

    std::map<sockaddr_in, time_t> clients;
    std::queue<std::pair<sockaddr_in, datagram>> responds;

public:

    Responds(std::string s) : file_content(s) {}

    void add_client(sockaddr_in client) {
        time_t timestamp;
        time(&timestamp);
        clients[client] = timestamp;
    }

    void remove_old_clients() {
        time_t timestamp;
        time(&timestamp);
        timestamp -= 120; // 2 minutes ago

        for (auto i = clients.cbegin(); i != clients.end(); i++) {
            if (i->second < timestamp)
                clients.erase(i);
        }
    }

    void generate_responds(datagram d) {
        remove_old_clients();

        d.text = file_content;
        for (auto &kv : clients)
            responds.push(std::make_pair(kv.first, d));
    }

    std::pair<sockaddr_in, datagram> get_respond() {
        std::pair<sockaddr_in, datagram> ret = responds.front();
        responds.pop();
        return ret;
    }

    bool empty() {
        return responds.empty();
    }
};

class Server {

    Receive receives;
    Responds *responds;

    datagram_base *receive_buffer;

    struct sockaddr_in server;
    struct pollfd client[MAX_SOCKETS];

public:

    Server(char *port, char *filename) {
        responds = new Responds(read_file(filename));
        receive_buffer = (datagram_base *)malloc(BUFFER_SIZE);

        client[0].events = POLLIN;
        client[0].fd = -1;
        for (int i = 1; i < MAX_SOCKETS; ++i) {
            client[i].fd = -1;
            client[i].events = POLLOUT;
        }

        client[0].fd = create_socket();

        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons(parse_port(port));
        if (bind(client[0].fd, (struct sockaddr *) &server, sizeof(server)) < 0) {
            perror("Binding stream socket");
            exit(EXIT_FAILURE);
        }

        ssize_t length = sizeof(server);
        if (getsockname (client[0].fd, (struct sockaddr*)&server, (socklen_t*)&length) < 0) {
            perror("Getting socket name");
            exit(EXIT_FAILURE);
        }

        for (int i = 1; i < MAX_SOCKETS; i++)
            client[i].fd = create_socket();
    }

    ~Server() {
        free(receive_buffer);
    }

    void run() {
        int events;
        nfds_t sockets;
        do {
            // generate responds if there aren't any
            if (responds->empty() && !receives.empty())
                generate_responds();

            if (responds->empty())
                sockets = 1;
            else
                sockets = MAX_SOCKETS;

            for (nfds_t i = 0; i < sockets; ++i)
                client[i].revents = 0;

            events = poll(client, sockets, 5000);
            if (events < 0) {
                perror("poll");
                continue;
            } else if (events == 0) { // no descriptors with events / errors
                continue;
            }

            if (client[0].revents & POLLIN)
                read();

            for (nfds_t i = 1; i < MAX_SOCKETS; ++i) {
                if (client[i].fd != -1 && (client[i].revents & POLLOUT))
                    pollout(i);
            }

        } while (true);
    }


private:

    std::string read_file(char *filename) {
        std::fstream file;
        file.open(filename, std::ios::in);
        if (!file.good()) {
            printf("Could not open file.");
            exit(EXIT_FAILURE);
        }

        std::string content;

        file.seekg(0, std::ios::end);
        content.reserve(file.tellg());

        file.seekg(0, std::ios::beg);
        content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        file.close();

        return content;
    }

    int create_socket() {
        int s = socket(PF_INET, SOCK_DGRAM, 0);
        if (s < 0) {
            perror("Opening stream socket");
            exit(EXIT_FAILURE);
        }
        return s;
    }

    void generate_responds() {
        if (receives.empty())
            return;

        std::pair<datagram, sockaddr_in> r = receives.pop();
        sockaddr_in client = r.second;
        datagram d = r.first;

        responds->generate_responds(d);
        responds->add_client(client);
    }

    void read() {
        sockaddr_in receive_address = {};
        datagram d;
        if (receive(client[0].fd, &receive_address, receive_buffer) < 0) {
            fprintf(stderr, "Reading datagram from client error.\n");
            return;
        }

        if (!parse_datagram(receive_buffer, &d)) {
            err_with_ip("[%s] Parsing datagram error\n", receive_address);
            return;
        }

        receives.add(receive_address, d);
    }

    void pollout(int i) {
        if (responds->empty())
            return;

        std::pair<sockaddr_in, datagram> r = responds->get_respond();
        send_datagram(i, r.first, r.second);
    }

    void send_datagram(int i, sockaddr_in send_address, datagram d) {
        size_t length = sizeof(d);
        if (send(client[i].fd, send_address, &d, length) != (ssize_t)length)
            err_with_ip("[%s] Sending datagram error", send_address);
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

    Server s = Server(argv[1], argv[2]);
    s.run();

    return EXIT_SUCCESS;
}
