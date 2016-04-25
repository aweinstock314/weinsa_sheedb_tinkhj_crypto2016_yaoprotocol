#include <cryptopp/filters.h>
#include <fcntl.h>
#include <sys/stat.h>
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
SenderGarbledCircuit::SenderGarbledCircuit(Circuit c_) :
    c(c_), zeros(c.wires.size()), ones(c.wires.size()), lambdas(c.wires.size()) {
    int fd = open("/dev/urandom", O_RDONLY);
    uint8_t temp;
    size_t i;

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
        //  it would be more elegant to walk the circuit starting from the outputs, but this is easier
        struct matcher : public boost::static_visitor<> {
            SenderGarbledCircuit *p;
            size_t i;
            matcher(SenderGarbledCircuit *parent, size_t i_) : p(parent), i(i_) {}
            void operator()(InputWire& w) const {
                p->gates.push_back(w);
            }
            void operator()(OutputWire& w) const {
                p->gates.push_back(w);
            }
            void operator()(GateWire& w) const {
                size_t j, k;
                for(j = 0; j <= 1; j++) { for(k = 0; k <= 1; k++) {
                    // adapted from Figure 1 of Zikas et. al.
                    // P[g, j, k] <- Enc_{K_a^j, K_b^k}(K_g^\sigma concat \sigma)
                    // \sigma = G_g(\lambda_a xor j, \lambda_b xor k) xor \lambda_g
                    GarbledWire gw;
                    p->gates.push_back(gw);
                    uint8_t *buf = &gw.values[j][k][0];
                    gw.l = w.l; gw.r = w.r;
                    bool sigma = eval_truthtable(w.truth_table, p->lambdas[w.l] ^ j, p->lambdas[w.r] ^ k) ^ p->lambdas[i];
#define KEY(wire, bit) ((bit) ? p->ones[(wire)] : p->zeros[(wire)]).data()
                    memcpy(buf, KEY(i, sigma), SEC_PARAM);
                    buf[SEC_PARAM] = sigma;
                    encrypt(buf, buf, KEY(w.l, j), KEY(w.r, k));
#undef KEY
                }}
            }
        };
        boost::apply_visitor(matcher(this, i), c.wires[i]);
    }
}

void serialize_gc(int fd, const vector<GarbledGate>& gc) {
    struct serializer : public boost::static_visitor<> {
        int fd;
        serializer(int fd_) : fd(fd_) {}
        void operator()(const InputWire& w) {
            write_aon(fd, "0", sizeof(char));
            write_aon(fd, tag_to_bool(w.who) ? "S" : "R", sizeof(char));
            write_aon(fd, (char*)w.index, sizeof(w.index));
        }
        void operator()(const GarbledWire& w) {
            write_aon(fd, "1", sizeof(char));
            write_aon(fd, (char*)&w.values[0][0][0], 2 * 2 * (SEC_PARAM+1)*sizeof(uint8_t));
            write_aon(fd, (char*)&w.l, sizeof(size_t));
            write_aon(fd, (char*)&w.r, sizeof(size_t));
        }
        void operator()(const OutputWire& w) {
            write_aon(fd, "2", sizeof(char));
            write_aon(fd, (char*)w.index, sizeof(w.index));
        }
    };
    size_t i, n = gc.size();
    serializer ser(fd);
    write_aon(fd, (char*)n, sizeof(size_t));
    for(i=0; i<n; i++) {
        boost::apply_visitor(ser, gc[i]);
    }
}

GarbledGate deserialize_gate(int fd) {
    char flag;
    read_aon(fd, &flag, sizeof(char));
    switch(flag) {
        case '0': {
            char who;
            read_aon(fd, &who, sizeof(char));
            size_t index;
            read_aon(fd, (char*)&index, sizeof(size_t));
            return InputWire(index, bool_to_tag(who == 'S' ? true : false));
        }
        case '1': {
            GarbledWire w;
            read_aon(fd, (char*)&w.values[0][0][0], 2 * 2 * (SEC_PARAM+1)*sizeof(uint8_t));
            read_aon(fd, (char*)&w.l, sizeof(size_t));
            read_aon(fd, (char*)&w.r, sizeof(size_t));
            return w;
        }
        case '2': {
            size_t index;
            read_aon(fd, (char*)&index, sizeof(size_t));
            return OutputWire(index);
        }
        default: {
            printf("deserialize_gate: wonky flag value %d\n", flag);
            exit(1);
        }
    }
}

vector<GarbledGate> deserialize_gc(int fd) {
    vector<GarbledGate> gc;
    size_t i, n;
    read_aon(fd, (char*)&n, sizeof(size_t));
    for(i=0; i<n; i++) {
        gc.push_back(deserialize_gate(fd));
    }
    return gc;
}

bitvector ReceiverGarbledCircuit::eval(const bitvector& y) {
    struct eval_garbled_wire : public static_visitor<> {
        ReceiverGarbledCircuit *p;
        const bitvector *y;
        bitvector *output;
        size_t i;
        eval_garbled_wire(ReceiverGarbledCircuit *parent, const bitvector *y_, bitvector *output_, size_t i_) : p(parent), y(y_), output(output_), i(i_) {}
        // TODO: support non-topological ordering (currently the asserts will fail if given a non-topological circuit)
        void operator()(const InputWire&) {
            assert(p->evaluated[i]);
        }
        void operator()(const GarbledWire& w) {
            assert(p->evaluated[w.l]);
            assert(p->evaluated[w.r]);
            p->evaluated[i] = true;
            uint8_t buf[SEC_PARAM+1];
            decrypt(buf, w.values[p->sigmas[w.l]][p->sigmas[w.r]], p->keys[w.l].data(), p->keys[w.r].data());
            p->sigmas[i] = buf[SEC_PARAM];
            memcpy(p->keys[i].data(), buf, SEC_PARAM);
        }
        void operator()(const OutputWire& w) {
            assert(p->evaluated[w.index]);
            output->push_back(p->sigmas[w.index]);
        }
    };
    size_t i;
    bitvector output;

    for(i=0; i<gates.size(); i++) {
        eval_garbled_wire egw(this, &y, &output, i);
        boost::apply_visitor(egw, gates[i]);
    }

    return output;
}
