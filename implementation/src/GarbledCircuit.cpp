#include <crypto++/filters.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "Circuit.h"
#include "GarbledCircuit.h"

void encrypt(uint8_t *dst, const uint8_t *src, const uint8_t *k1, const uint8_t *k2) {
    GARBLER_CIPHER::Encryption e1(k1, SEC_PARAM), e2(k2, SEC_PARAM);
    CryptoPP::ArraySink sink(dst, SEC_PARAM+1);
    CryptoPP::StreamTransformationFilter stf1(e1, &sink), stf2(e2, &sink);
    stf2.PutMessageEnd(src, SEC_PARAM+1);
    stf1.PutMessageEnd(src, SEC_PARAM+1);
}

void decrypt(uint8_t *dst, const uint8_t *src, const uint8_t *k1, const uint8_t *k2) {
    GARBLER_CIPHER::Decryption e1(k1, SEC_PARAM), e2(k2, SEC_PARAM);
    CryptoPP::ArraySink sink(dst, SEC_PARAM+1);
    CryptoPP::StreamTransformationFilter stf1(e1, &sink), stf2(e2, &sink);
    stf1.PutMessageEnd(src, SEC_PARAM+1);
    stf2.PutMessageEnd(src, SEC_PARAM+1);
}

// This uses the "Single-Party Garbling Scheme" described in section 3.1 of the paper
//  "Efficient Three-Party Computation from Cut-and-Choose" by Choi, Katz, Malozemoff, and Zikas
// This is more efficient than the garbling scheme from "A Proof of Security of Yao's Protocol for Two-Party Computation" by Lindell and Pinkas
//  since the receiver only needs to make 1 decryption per gate (down from 4), as well as space/communication savings from lack of padding/output maps
GarbledCircuit::GarbledCircuit(Circuit c_) :
    c(c_), zeros(c.wires.size()), ones(c.wires.size()), gates(c.wires.size()), lambdas(c.wires.size()) {
    int fd = open("/dev/urandom", O_RDONLY);
    uint8_t temp;
    size_t i,j,k;
    GateWire *w;

    for (i=0; i<c.wires.size(); i++) {
        // produce random bytevectors for the keys
        zeros[i].resize(SEC_PARAM);
        ones[i].resize(SEC_PARAM);
        read_aon(fd, (char*)zeros[i].data(), SEC_PARAM);
        read_aon(fd, (char*)ones[i].data(), SEC_PARAM);


        if(boost::get<OutputWire>(&c.wires[i])) {
            // the output wires should not be blinded
            lambdas[i] = 0;
        } else {
            // produce random bit for the blindings
            read_aon(fd, (char*)&temp, 1);
            // Extract a single bit of randomness from a full byte
            // test is correct to extract the MSB without bias, as demonstrated by the following haskell
            // ghci> length[0..127]==length[128..255]
            // True
            lambdas[i] = temp<128 ? 1 : 0;
        }

        // assumes a topological ordering, which is produced by generate_unsigned_compare_circuit
        //  it would be more elegant to walk the circuit starting from the outputs, but this is easier (because boost)
        if((w = boost::get<GateWire>(&c.wires[i]))) {
            for(j = 0; j <= 1; j++) { for(k = 0; k <= 1; k++) {
                // adapted from Figure 1 of Zikas et. al.
                // P[g, j, k] <- Enc_{K_a^j, K_b^k}(K_g^\sigma concat \sigma)
                // \sigma = G_g(\lambda_a xor j, \lambda_b xor k) xor \lambda_g
                uint8_t *buf = &gates[i].values[j][k][0];
                bool sigma = eval_truthtable(w->truth_table, lambdas[w->l] ^ j, lambdas[w->r] ^ k) ^ lambdas[i];
#define KEY(wire, bit) ((bit) ? ones[(wire)] : zeros[(wire)]).data()
                memcpy(buf, KEY(i, sigma), SEC_PARAM);
                buf[SEC_PARAM] = sigma;
                encrypt(buf, buf, KEY(w->l, j), KEY(w->r, k));
#undef KEY
            }}
        }
    }

    // walk circuit, gen encrypted truth tables
    // use eval_truthtable to get output value
    // add bit vec for lambdas

// TODO: init 0s and 1s inside contructor
// create garbled tables
}

template<class OT> void recv(int fd, bytevector y_) {
    bitvector y = unpack_bv(y_);
    // TODO: recive and eval circuit
}

template<class OT> void GarbledCircuit::send(int fd, bytevector x_) {
    bitvector x = unpack_bv(x_);
    size_t i;
    for(i=0; i<x.size(); i++) {
        write_aon(fd, (x[i] ? ones[i] : zeros[i]).data(), SEC_PARAM);
    }
    // x.size() == y.size() invariant should be enforced elsewhere
    for(i=0; i<x.size(); i++) {
        OT::send(fd, zeros[i], ones[i]); // todo: architechture for parallelism
    }
}

//send<RSA OT>( // todo: args);
//send<DUAL_EC_DRBG_OT>(// todo: args);
