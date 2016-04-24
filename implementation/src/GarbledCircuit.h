#ifndef GARBLED_CIRCUIT_H
#define GARBLED_CIRCUIT_H

#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/salsa.h>
#include "Circuit.h"
#include "utils.h"

// statistical security parameter
#define SEC_PARAM (128/8)

// TODO: unhackify with templates, test performance of both AES and Salsa20
#define GARBLER_CIPHER CryptoPP::CTR_Mode<CryptoPP::AES>
//#define GARBLER_CIPHER CryptoPP::Salsa20

void encrypt(uint8_t *dst, const uint8_t *src, const uint8_t *k1, const uint8_t *k2);
void decrypt(uint8_t *dst, const uint8_t *src, const uint8_t *k1, const uint8_t *k2);

struct GarbledWire {
    uint8_t values[2][2][SEC_PARAM+1];
    size_t l, r;
};

typedef variant<InputWire, GarbledWire, OutputWire> GarbledGate;

struct SenderGarbledCircuit {
    // c.wires.size() == zeros.size() && zeros.size() == ones.size()
    // zeros[i] \eqv K_i^0
    Circuit c;
    vector<bytevector> zeros, ones;
    vector<GarbledGate> gates;
    bitvector lambdas;

    SenderGarbledCircuit(Circuit c_);

    template<class OT> bytevector send(int fd, bytevector x_);
};

struct ReceiverGarbledCircuit {
    vector<bytevector> keys;
    bitvector result;
    vector<GarbledGate> gates;
    size_t num_bits;
    bitvector lambdas;
    bitvector evaluated;
    bitvector sigmas;

    template<class OT> ReceiverGarbledCircuit(int fd, bytevector y_);
    bitvector eval(const bitvector& y);
};

void serialize_gc(int fd, const vector<GarbledGate>& gc);
GarbledGate deserialize_gate(int fd);
vector<GarbledGate> deserialize_gc(int fd);

#endif
