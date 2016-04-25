#include <unistd.h>
#include "utils.h"

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

bitvector unpack_bv(bytevector x) {
    bitvector y;
    for(uint8_t a : x) {
        for(int i = 0; i < 8; i++){
            y.push_back( !!( (a & (1 << i)) >> i) );
        }
    }
    return y;
}

bytevector pack_bv(bitvector x) {
    bytevector y;
    uint8_t tmp=0, i=0;
    for(bool a : x) {
        tmp |= (a << i);
        if(++i == 8) {
            y.push_back(tmp);
            tmp = i = 0;
        }
    }
    if(i != 0) { y.push_back(tmp); }
    return y;
}

void nopprintf(FILE*, const char*, ...) {}

void print_bytevector_as_bits(bytevector bv_) {
    bitvector bv = unpack_bv(bv_);
    size_t i, n=bv.size();
    for(i=0; i<n; i++) {
        printf("%d", !!bv[n-i-1]);
    }
}
