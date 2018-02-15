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
#include <sstream>

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

    std::map<sockaddr_in, time_t> clients;
    std::queue<std::pair<sockaddr_in, datagram>> responds;

public:

    void add_client(sockaddr_in client) {
        time_t timestamp;
        time(&timestamp);
        clients[client] = timestamp;
    }

    void remove_old_clients() {
        time_t timestamp;
        time(&timestamp);
        timestamp -= 120; // 2 minutes ago

        for (auto i = clients.cbegin(); i != clients.cend();) {
            if (i->second < timestamp)
                clients.erase(i++);
            else
                ++i;
        }
    }

    void generate_responds(datagram d) {
        remove_old_clients();

        for (auto i = clients.begin(); i != clients.end(); ++i)
            responds.push(std::make_pair(i->first, d));
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
    Responds responds;

    std::string file_content;

    datagram_base *receive_buffer;

    struct sockaddr_in server;
    struct pollfd client[MAX_SOCKETS];

public:

    Server(char *port, char *filename) {

        file_content = read_file(filename);
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

        for (auto &c : client)
            close(c.fd);
    }

    void run() {
        int events;
        nfds_t sockets;
        do {
            if (responds.empty() && !receives.empty())
                generate_responds();

            if (responds.empty())
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

            if (client[0].revents & POLLIN) {
                read();
            }

            for (nfds_t i = 1; i < sockets; ++i) {
                if (client[i].fd != -1 && (client[i].revents & POLLOUT))
                    pollout(i);
            }

        } while (true);
    }


private:

    std::string read_file(char *filename) {
        std::string content;
        
        std::ifstream file(filename);
        content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

        file.close();

        if (content.size() + sizeof(datagram_base) > BUFFER_SIZE) {
            fprintf(stderr, "Given file is too big for UDP packed.\n");
            exit(EXIT_FAILURE);
        }

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

        datagram d;
        sockaddr_in client;
        bool valid = false;
        std::pair<datagram, sockaddr_in> r;

        while (!valid) {
            r = receives.pop();
            client = r.second;
            d = r.first;

            valid = timestamp_valid(d.timestamp);
        }

        responds.add_client(client);

        if (valid)
            responds.generate_responds(d);
    }

    void read() {
        sockaddr_in receive_address;
        datagram d;

        size_t bytes_received = receive(client[0].fd, &receive_address, receive_buffer); 

        if (bytes_received < 0) {
            fprintf(stderr, "Reading datagram from client error.\n");
            return;
        }

        parse_datagram(receive_buffer, &d, bytes_received);
        receives.add(receive_address, d);
    }

    void pollout(int i) {
        if (responds.empty())
            return;

        std::pair<sockaddr_in, datagram> r = responds.get_respond();
        send_datagram(i, r.first, r.second);
    }

    void send_datagram(int i, sockaddr_in send_address, datagram d) {
        prepare_datagram_to_send(&d);
        d.text = file_content;

        std::string to_send = datagram_to_string(d);

        size_t length = to_send.size();
        if (send(client[i].fd, send_address, to_send.data(), length) != (ssize_t)length)
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
