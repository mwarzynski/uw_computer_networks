#ifndef DATAGRAM_H
#define DATAGRAM_H

#include <stdlib.h>
#include <string>
#include <time.h>
#include <vector>
#include <regex>
#include <cstdbool>
#include <cctype>

struct datagram {
    uint64_t timestamp;
    char character;
    char text[];
};


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

    return true;
}

#endif //DATAGRAM_H
