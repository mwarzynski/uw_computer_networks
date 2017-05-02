#ifndef DATAGRAM_H
#define DATAGRAM_H

#define DATAGRAM_INVALID 1

#include <stdlib.h>
#include <string>
#include <time.h>
#include <vector>


size_t create_datagram(char *result, char *text, char character) {
    const char *format = "%ld %s%c";
    memset(result, 0, sizeof(result));
    time_t raw_time;
    time(&raw_time);

    unsigned long length = (unsigned long)snprintf(nullptr, 0, format, raw_time, text, character);
    snprintf(result, length + 1, format, raw_time, text, character);
    return length + 1;
}

int parse_datagram(char *result, uint64_t *timestamp, char *text) {
    return 0;
}

#endif //DATAGRAM_H
