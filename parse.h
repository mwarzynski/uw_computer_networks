#ifndef PARSE_H
#define PARSE_H

#include <cstdint>
#include <string>
#include <stdexcept>
#include <arpa/inet.h>

bool parse_host(char *text, sockaddr_in *my_address) {
    struct addrinfo address_hints;
    struct addrinfo *address_result;

    memset(&address_hints, 0, sizeof(struct addrinfo));
    address_hints.ai_family = AF_INET;
    address_hints.ai_socktype = SOCK_DGRAM;
    address_hints.ai_protocol = IPPROTO_UDP;
    address_hints.ai_flags = 0;
    address_hints.ai_addrlen = 0;
    address_hints.ai_addr = NULL;
    address_hints.ai_canonname = NULL;
    address_hints.ai_next = NULL;

    if (getaddrinfo(text, NULL, &address_hints, &address_result) != 0)
        return false;

    my_address->sin_family = AF_INET;
    my_address->sin_addr.s_addr = ((struct sockaddr_in*)address_result->ai_addr)->sin_addr.s_addr;

    freeaddrinfo(address_result);
    return true;
}

uint16_t parse_port(char *text) {
    uint16_t port;

    try {
        port = (uint16_t)std::stoi(text);
    } catch (...) {
        port = 0;
    }

    if (port < 1 || 65535 < port)
        port = 0;

    return port;
}

void err_with_ip(std::string message, sockaddr_in sa) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(sa.sin_addr), ip, INET_ADDRSTRLEN);
    fprintf(stderr, "Error: %s%s\n", message.c_str(), ip);
}

#endif // PARSE_H
