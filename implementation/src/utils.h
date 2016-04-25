#ifndef UTILS_H
#define UTILS_H
#include <boost/utility/binary.hpp>
#include <boost/variant.hpp>
#include <stdint.h>
#include <vector>

typedef std::vector<uint8_t> bytevector;
typedef std::vector<bool> bitvector;

// "all or nothing" read/write, to handle interruptions/byte-by-byte input
size_t read_aon(int fd, char* buf, size_t count);
size_t write_aon(int fd, const char* buf, size_t count);

//Unpacks a bytevector into a bitvector with LSB first
bitvector unpack_bv(bytevector x);
//Packs a bit vector into a bytevector
bytevector pack_bv(bitvector x);

template <class T> struct PhantomData {};
#endif
