#include <unistd.h>
#include "utils.h"

// "all or nothing" read/write, to handle interruptions/byte-by-byte input
#define DEF_AON(prefix, underlying) \
size_t underlying ## _aon(int fd, prefix char* buf, size_t count) { \
    ssize_t tmp; \
    size_t sofar = 0; \
    while(sofar < count) { \
        tmp = underlying(fd, buf+sofar, count-sofar); \
        if(tmp <= 0) { return -1; } \
        sofar += tmp; \
    } \
    return sofar; \
}

DEF_AON(,read)
DEF_AON(const, write)

#undef DEF_AON


