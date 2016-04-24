#ifndef GARBLED_CIRCUIT_H
#define GARBLED_CIRCUIT_H

#include "utils.h"

// statistical security parameter
#define SEC_PARAM (128)

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
    GarbledCircuit::GarbledCircuit(Circuit c_);

    // TODO: create recv's constructor
    template<class OT> void send(int fd, bytevector x_);
};

#endif
