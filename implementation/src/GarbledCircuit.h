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

    template<class OT> ReceiverGarbledCircuit(PhantomData<OT> ot, int fd, bytevector y_);
    bitvector eval(const bitvector& y);
};

void serialize_gc(int fd, const vector<GarbledGate>& gc);
GarbledGate deserialize_gate(int fd);
vector<GarbledGate> deserialize_gc(int fd);

template<class OT> bytevector SenderGarbledCircuit::send(int fd, bytevector x_) {
    bitvector x = unpack_bv(x_);
    assert(x.size() == c.num_bits);
    size_t i;
    write_aon(fd, (char*)&c.num_bits, sizeof(size_t));
    serialize_gc(fd, gates);
    for(i=0; i<c.num_bits; i++) {
        bool sigma = x[i] ^ lambdas[i];
        write_aon(fd, (char*)&sigma, sizeof(bool));
    }
    for(i=0; i<c.num_bits; i++) {
        bool tmp = lambdas[i+c.num_bits];
        write_aon(fd, (char*)&tmp, sizeof(bool));
    }
    for(i=0; i<c.num_bits; i++) {
        write_aon(fd, (char*)(x[i] ^ lambdas[i] ? ones[i] : zeros[i]).data(), SEC_PARAM);
    }
    for(i=0; i<c.num_bits; i++) {
        OT::send(fd, zeros[i], ones[i]); // todo: architechture for parallelism
    }

    bytevector result;
    size_t output_size;
    read_aon(fd, (char*)&output_size, sizeof(size_t));
    read_aon(fd, (char*)result.data(), output_size);

    return result;
}

template<class OT> ReceiverGarbledCircuit::ReceiverGarbledCircuit(PhantomData<OT>, int fd, bytevector y_) {
    size_t i;
    bitvector y = unpack_bv(y_);
    read_aon(fd, (char*)&num_bits, sizeof(size_t));
    assert(y.size() <= num_bits);
    if(y.size() < num_bits) {
        y.resize(num_bits);
    }

    gates = deserialize_gc(fd);

    evaluated.resize(gates.size(), 0);
    sigmas.resize(gates.size(), 0);
    keys.resize(gates.size(), vector<uint8_t>(SEC_PARAM, 0));
    lambdas.resize(num_bits);

    for(i=0; i<num_bits; i++) {
        bool sigma;
        read_aon(fd, (char*)&sigma, sizeof(bool));
        sigmas[i] = sigma;
    }
    for(i=0; i<num_bits; i++) {
        bool tmp;
        read_aon(fd, (char*)&tmp, sizeof(bool));
        lambdas[i] = tmp;
        sigmas[num_bits+i] = y[i] ^ lambdas[i];
    }
    for(i=0; i<num_bits; i++) {
        keys[i].resize(SEC_PARAM);
        write_aon(fd, (char*)keys[i].data(), SEC_PARAM);
        evaluated[i] = true;
    }
    for(i=0; i<num_bits; i++) {
        keys[i+num_bits] = OT::recv(fd, y[i] ^ lambdas[i]);
        evaluated[i+num_bits] = true;
    }

    evaluated.resize(num_bits, false);

    result = eval(y);

    bytevector result_ = pack_bv(result);
    size_t tmp = result_.size();
    write_aon(fd, (char*)&tmp, sizeof(size_t));
    write_aon(fd, (char*)result_.data(), tmp);
}
#endif
