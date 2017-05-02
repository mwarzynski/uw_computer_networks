#ifndef DATAGRAM_H
#define DATAGRAM_H

#include <stdlib.h>
#include <time.h>
#include <cstdbool>
#include <cctype>
#include <string>

struct datagram {
    uint64_t timestamp;
    char character;
    std::string text;
};

void prepare_datagram(datagram *d, char character, std::string text) {
    d->character = character;

    time_t raw_time; time(&raw_time);
    d->timestamp = (uint64_t)raw_time;
    d->text = text;
}

bool parse_datagram(datagram *d) {
    if (!d)
        return false;

    // check if character is a letter
    if (!isalpha(d->character))
        return false;

    // parse timestamp
    time_t raw_time = (time_t)d->timestamp;
    struct tm *ptm = gmtime(&raw_time);

    // ptm->tm_year = years since 1900
    int year = ptm->tm_year + 1900;

    // check if year is within given range (QUESTION: how the fuck can it be lower than 1717?)
    if (year < 1717 || 4242 < year)
        return false;

    for (int i = 0; d->text.size(); i++) {
        if (!isalpha(d->text[i]))
            return false;
    }

    return true;
}

ssize_t send(int sock, sockaddr_in destination, void *data, size_t length) {
    struct sockaddr * send_sock_addr = (struct sockaddr *)&destination;
    socklen_t send_addr_length = (socklen_t)sizeof(destination);
    return sendto(sock, data, length, 0, send_sock_addr, send_addr_length);
}

ssize_t receive(int sock, sockaddr_in receive_address, void *data, size_t length) {
    struct sockaddr * receive_sock_addr = (struct sockaddr *)&receive_address;
    socklen_t receive_addr_length = (socklen_t)sizeof(receive_address);
    return recvfrom(sock, data, length, 0, receive_sock_addr, &receive_addr_length);
}

#endif //DATAGRAM_H
