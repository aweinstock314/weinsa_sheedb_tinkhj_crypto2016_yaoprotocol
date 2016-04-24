#ifndef GARBLED_CIRCUIT_H
#define GARBLED_CIRCUIT_H

#include <crypto++/aes.h>
#include <crypto++/modes.h>
#include <crypto++/salsa.h>
#include "Circuit.h"
#include "utils.h"

// statistical security parameter
#define SEC_PARAM (128/8)

// TODO: unhackify with templates, test performance of both AES and Salsa20
#define GARBLER_CIPHER CryptoPP::CTR_Mode<CryptoPP::AES>
//#define GARBLER_CIPHER CryptoPP::Salsa20


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

    template<class OT> void send(int fd, bytevector x_);
};

struct ReceiverGarbledCircuit {
    vector<bytevector> keys;
    vector<GarbledGate> gates;
    bitvector lambdas;

    ReceiverGarbledCircuit(Circuit c_);

    template<class OT> void recv(int fd, bytevector y_);
};

#endif
