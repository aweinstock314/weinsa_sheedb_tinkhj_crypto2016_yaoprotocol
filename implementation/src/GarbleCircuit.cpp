#include <fcntl.h>
#include <sys/stat.h>
#include "utils.h"

#define SEC_PARAM (128)

struct GarbledGate {
    uint8_t values[2][2][SEC_PARAM+1];
};

// This uses the "Single-Party Garbling Scheme" described in section 3.1 of the paper
//  "Efficient Three-Party Computation from Cut-and-Choose" by Choi, Katz, Malozemoff, and Zikas
// This is more efficient than the garbling scheme from "A Proof of Security of Yao's Protocol for Two-Party Computation" by Lindell and Pinkas
//  since the receiver only needs to make 1 decryption per gate (down from 4), as well as space/communication savings from lack of padding/output maps
struct GarbledCircuit {
    // c.wires.size() == zeros.size() && zeros.size() == ones.size()
    // zeros[i] \eqv K_i^0 
    Circuit c; 
    vector<bytevector> zeros, ones; 
    vector<GarbledGate> gates; 
    bitvector lambdas; 

    // sender's constructor
    GarbledCircuit(Circuit c_) : c(c_) , zeros(c.wires.size()), ones(c.wires.size()), gates(c.wires.size()), lambdas(c.wires.size()) {
        int fd = open("/dev/urandom", O_RDONLY);
        uint8_t temp;

        for (size_t i=0; i<c.wires.size(); i++) {
            zeros[i].resize(SEC_PARAM);
            ones[i].resize(SEC_PARAM);
            read(fd, zeros[i].data(), SEC_PARAM);
            read(fd, ones[i].data(), SEC_PARAM);
            read(fd, &temp, 1);

            // Extract a single bit of randomness from a full byte
            // test is correct to extract the MSB without bias, as demonstrated by the following haskell
			// ghci> length[0..127]==length[128..255]
			// True
            lamdas[i] = temp<128 ? 1 : 0;
        }

        // walk circuit, gen encrypted truth tables
        // use eval_truthtable to get output value
        // add bit vec for lambdas 
        
    // TODO: init 0s and 1s inside contructor
    // create garbled tables
    }

    // TODO: create recv's constructor
    template<class OT> void send(int fd, bytevector x_);

};


template<class OT> void recv(int fd, bytevector y_) {
    bitvector y = unpack_bv(y_);
    // TODO: recive and eval circuit
}

template<class OT> void GarbledCircuit::send(int fd, bytevector x_) {
    bitvector x = unpack_bv(x_);
    size i;
    for(i=0, i<x.size(); i++) {
        write(fd, x[i] ? ones[i] : zeros[i], SEC_PARAM);
    }
    for(i=0; i<reciever_bits; i++) {
        OT::Send(fd, zeros[i], ones[i]); // todo: parallelism 
    }
}
        

//send<RSA OT>( // todo: args);
//send<DUAL_EC_DRBG_OT>(// todo: args);
