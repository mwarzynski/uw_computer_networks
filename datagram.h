#ifndef DATAGRAM_H
#define DATAGRAM_H

#include <stdlib.h>
#include <time.h>
#include <cstdbool>
#include <cctype>
#include <string>
#include <array>

#define BUFFER_SIZE 66000

struct datagram_base {
    uint64_t timestamp;
    char character;
} __attribute__((packed));

struct datagram {
    uint64_t timestamp;
    char character;
    std::string text;
};

void print_datagram(datagram *d) {
    printf("%ld%c%s", d->timestamp, d->character, d->text.c_str());
    fflush(stdout);
}

void prepare_datagram(datagram *d, char character, std::string text) {
    d->character = character;

    time_t raw_time; time(&raw_time);
    d->timestamp = __builtin_bswap64((uint64_t)raw_time);
    d->text = text;
}

bool parse_datagram(datagram_base *b, datagram *d) {
    if (!b)
        return false;

    if (!isalpha(b->character))
        return false;

    b->timestamp = __builtin_bswap64(b->timestamp);

    time_t raw_time = (time_t)b->timestamp;
    struct tm *ptm = gmtime(&raw_time);

    if (!ptm)
        return false;

    // ptm->tm_year = years since 1900
    int year = ptm->tm_year + 1900;

    // TODO: delete 'the fuck' from the line below
    // check if year is within given range (QUESTION: how the fuck can it be lower than 1717?)
    if (year < 1717 || 4242 < year)
        return false;

    d->timestamp = b->timestamp;
    d->character = b->character;
    d->text = std::string((char *)(b) + sizeof(datagram_base));

    return true;
}

ssize_t send(int sock, sockaddr_in destination, void *data, size_t length) {
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
