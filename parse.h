#ifndef PARSE_H
#define PARSE_H

#include <cstdint>
#include <string>
#include <stdexcept>

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

#endif // PARSE_H
