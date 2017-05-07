#ifndef DATAGRAM_H
#define DATAGRAM_H

#include <stdlib.h>
#include <time.h>
#include <cstdbool>
#include <cctype>
#include <string>
#include <sstream>

#define BUFFER_SIZE 65507

struct datagram_base {
    uint64_t timestamp;
    uint8_t character;
} __attribute__((packed));

struct datagram {
    uint64_t timestamp;
    char character;
    std::string text;
};

std::string datagram_to_string(datagram d) {
    std::stringstream s;

    datagram_base base;
    base.timestamp = d.timestamp;
    base.character = d.character;

    s.write(reinterpret_cast<char*>(&base), sizeof(datagram_base));
    s.write(d.text.data(), d.text.size());
    s.write("\0", sizeof(char));

    return s.str();
}

void print_datagram(datagram *d) {
    printf("%ld %c %s", d->timestamp, d->character, d->text.c_str());
    fflush(stdout);
}

void prepare_datagram_to_send(datagram *d) {
    d->timestamp = __builtin_bswap64(d->timestamp);
}

bool timestamp_valid(time_t timestamp) {
    struct tm *ptm = gmtime(&timestamp);

    if (!ptm)
        return false;

    // ptm->tm_year = years since 1900
    uint32_t year = ptm->tm_year + 1900;

    // check if year is within given range (QUESTION: how can it be lower than 1717?)
    if (year < 1717 || 4242 < year)
        return false;

    return true;
}

void parse_datagram(datagram_base *b, datagram *d, size_t bytes_received) {
    b->timestamp = __builtin_bswap64(b->timestamp);
    d->timestamp = b->timestamp;
    d->character = b->character;

    char *text = reinterpret_cast<char*>((char *)(b) + sizeof(datagram_base));
    d->text = std::string(text, bytes_received - sizeof(datagram_base) - 1);
}

ssize_t send(int sock, sockaddr_in destination, const void *data, size_t length) {
    struct sockaddr * send_sock_addr = (struct sockaddr *)&destination;
    socklen_t send_addr_length = (socklen_t)sizeof(destination);
    return sendto(sock, data, length, 0, send_sock_addr, send_addr_length);
}

ssize_t receive(int sock, sockaddr_in *receive_address, void *data) {
    struct sockaddr *receive_sock_addr = (struct sockaddr *)receive_address;
    socklen_t receive_addr_length = (socklen_t)sizeof(*receive_address);
    ssize_t r = recvfrom(sock, data, BUFFER_SIZE, 0, receive_sock_addr, &receive_addr_length);
    return r;
}

#endif //DATAGRAM_H
