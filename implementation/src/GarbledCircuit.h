#ifndef GARBLED_CIRCUIT_H
#define GARBLED_CIRCUIT_H

#include <crypto++/aes.h>
#include <crypto++/modes.h>
#include <crypto++/salsa.h>
#include "utils.h"

// statistical security parameter
#define SEC_PARAM (128/8)

// TODO: unhackify with templates, test performance of both AES and Salsa20
#define GARBLER_CIPHER CryptoPP::CTR_Mode<CryptoPP::AES>
//#define GARBLER_CIPHER CryptoPP::Salsa20


struct GarbledGate {
    uint8_t values[2][2][SEC_PARAM+1];
};

struct GarbledCircuit {
    // c.wires.size() == zeros.size() && zeros.size() == ones.size()
    // zeros[i] \eqv K_i^0 
    Circuit c; 
    vector<bytevector> zeros, ones; 
    vector<GarbledGate> gates; 
    bitvector lambdas; 

    // sender's constructor
    GarbledCircuit(Circuit c_);

    // TODO: create recv's constructor
    template<class OT> void send(int fd, bytevector x_);
};

#endif
