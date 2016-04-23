#include <fcntl.h>
#include <sys/stat.h>
#include "utils.h"

#define SEC_PARAM (128)
struct GarbledGate {
    uint8_t values[2][2][SEC_PARAM+1];
};

struct GarbledCircuitNaive {
    // c.wires.size() == zeros.size() && zeros.size() == ones.size()
    // zeros[i] \eqv K_i^0 
    Circuit c; 
    vector<bytevector> zeros, ones; 
    vector<GarbledGate> gates; 
    bitvector lambdas; 

    // sender's constructor
    GarbledCircuitNaive(Circuit c_) : c(c_) , zeros(c.wires.size()), ones(c.wires.size()), gates(c.wires.size()), lambdas(c.wires.size()) {
        int fd = open("/dev/urandom", O_RDONLY);
        uint8_t temp;

        for (size_t i=0; i<c.wires.size(); i++) {
            zeros[i].resize(SEC_PARAM);
            ones[i].resize(SEC_PARAM);
            read(fd, zeros[i].data(), SEC_PARAM);
            read(fd, ones[i].data(), SEC_PARAM);
            read(fd, &temp, 1);

			// ghci> length[0..127]==length[128..255]
			// True
            lamdas[i] = temp<128 ? 1 : 0;
        }

        // walk circuit, gen encrypted truth tables
        // use eval_truthtable to get output value
        // add bit vec for lambdas 
        // will add citiations  
        
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

template<class OT> void GarbledCircuitNaive::send(int fd, bytevector x_) {
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
