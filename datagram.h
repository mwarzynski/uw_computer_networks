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

void print_datagram(datagram *d) {
    if (!d)
        return;
    printf("%ld%c%s", d->timestamp, d->character, d->text.c_str());
}

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

#endif //DATAGRAM_H
