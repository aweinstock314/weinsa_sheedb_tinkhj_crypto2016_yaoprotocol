#include <boost/variant.hpp>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include "utils.h"

using namespace boost;
using namespace std;

typedef std::vector<uint8_t> bytevector;
typedef std::vector<bool> bitvector;

bitvector unpack_bv(bytevector x) {
    bitvector y;
    for(uint8_t a : x) {
        for(uint8_t i=0; i<8;i++) {
            y.push_back(!!((a & (1 << i)) >> i));
        }
    }
    return y;
}

bytevector pack_bv(bitvector x) {
    bytevector y;
    uint8_t tmp=0, i=0;
    for(bool a : x) {
        tmp |= a;
        tmp <<= 1;
        if(++i == 8) {
            y.push_back(tmp);
            tmp = i = 0;
        }
    }
    if(i != 0) { y.push_back(tmp); }
    return y;
}

// placeholder naive implementation for testing
class TerriblyInsecureObliviousTransfer {
    public:
    static void send(int fd, bytevector x, bytevector y) {
        size_t len = std::min(x.size(), y.size());
        write_aon(fd, (char*)&len, sizeof(size_t));
        write_aon(fd, (char*)x.data(), len);
        write_aon(fd, (char*)y.data(), len);
    }
    static bytevector receive(int fd, uint8_t bit) {
        size_t len;
        read_aon(fd, (char*)&len, sizeof(size_t));
        bytevector x(len,0), y(len,0);
        read_aon(fd, (char*)x.data(), len);
        read_aon(fd, (char*)y.data(), len);
        return bit ? y : x;
    }
};


/******************************************************************************/
/* Wire and related structs ***************************************************/
/******************************************************************************/

// Derived from this haskell datastructure
// data Gate = Input Bool Int | Function (Bool->Bool->Bool) Gate Gate | Output Gate deriving (Eq, Show)

struct SenderTag {};
struct ReceiverTag {};

struct InputWire {
    InputWire(variant<SenderTag, ReceiverTag> who_) : who(who_) {}
    variant<SenderTag, ReceiverTag> who;
};
struct GateWire {
    GateWire(uint8_t truth_table_, size_t l_, size_t r_) : truth_table(truth_table_), l(l_), r(r_) {}
    uint8_t truth_table; size_t l; size_t r;
};
struct OutputWire {
    OutputWire(size_t index_) : index(index_) {}
    size_t index;
};
typedef variant<InputWire, GateWire, OutputWire> Wire;


/******************************************************************************/
/* Circuit class **************************************************************/
/******************************************************************************/
class Circuit {
public:
    size_t num_bits;
    vector<Wire> wires;

    Circuit(size_t num_bits);
    size_t add_gate(uint8_t truth_table, size_t l, size_t r) {
        wires.push_back(GateWire(truth_table, l, r));
        return wires.size() - 1;
    }
    void mark_as_output(size_t index) {
        wires.push_back(OutputWire(index));
    }
};

Circuit::Circuit(size_t num_bits){
    this->num_bits = num_bits;
    for(size_t i = 0; i < num_bits; i++) {
        wires.push_back(InputWire(SenderTag()));
    }
    for(size_t i = 0; i < num_bits; i++) {
        wires.push_back(InputWire(ReceiverTag()));
    }
}

/*bytevector eval_circuit(Circuit c, bytevector x_, bytevector y_) {
    bitvector result;
    struct eval_wire : public static_visitor<> {
        bitvector* output;
        bitvector x, y;
        eval_wire(bitvector* out, bitvector x_, bitvector y_) : output(o), x(x_), y(y_) {}
        bool operator()(InputWire& w) {
            if(w.who.get<SenderTag>()) {
            } else {
            }
        };
    } eval_wire(&result, bv_unpack(x_), bv_unpack(y_));
    size_t i;
    for(i=0; i<c.wires.size(); i++) {
        tmp[i] = apply_visitor(eval_wire, c.wires[i]);
    }
    return bv_pack(result);
}*/

/******************************************************************************/
/* The fabled generate_unsigned_compare_circuit function **********************/
/* It generates a circuit that calculates unsigned comparisons ****************/
/******************************************************************************/

// Derived from the following haskell
/*
mkManualLessThan size = output where
    primitive f i = Function f (Input False i) (Input True i)
    lessthans = map (primitive (<)) [size-1,size-2..0]
    xnors = Constant True : map (primitive ((not .) . xor)) [size-1,size-2..0]
    intermediate1 = Constant True : zipWith (Function (&&)) intermediate1 (tail xnors)
    intermediate2 = zipWith3 (\x y z -> (Function (&&) x (Function (&&) y z))) lessthans xnors intermediate1
    output = foldr (Function (||)) (Constant False) intermediate2
*/

Circuit generate_unsigned_compare_circuit(size_t num_bits) {
    Circuit unsigned_compare { num_bits };
    uint8_t and_table      = 0b1000;
    uint8_t false_table    = 0b0000;
    uint8_t lessthan_table = 0b0010;
    uint8_t or_table       = 0b1110;
    uint8_t true_table     = 0b1111;
    uint8_t xnor_table     = 0b1001;

#define SENDER_BIT(k) (k)
#define RECVER_BIT(k) (num_bits + (k))
    size_t i, tmp;
    vector<size_t> lessthans;
    for(i=0; i<num_bits; i++) {
        lessthans.push_back(unsigned_compare.add_gate(lessthan_table, SENDER_BIT(i), RECVER_BIT(i)));
    }
    vector<size_t> xnors;
    xnors.push_back(unsigned_compare.add_gate(true_table, 0, 0));
    for(i=num_bits-1; i!=(size_t)-1; i--) {
        xnors.push_back(unsigned_compare.add_gate(xnor_table, SENDER_BIT(i), RECVER_BIT(i)));
    }

    vector<size_t> intermediate1;
    intermediate1.push_back(unsigned_compare.add_gate(true_table, 0, 0));
    for(i=1; i<num_bits; i++) {
        intermediate1.push_back(unsigned_compare.add_gate(and_table, intermediate1[i-1], xnors[i]));
    }

    vector<size_t> intermediate2;
    for(i=0; i<num_bits; i++) {
        tmp = unsigned_compare.add_gate(and_table, lessthans[i], xnors[i]);
        intermediate2.push_back(unsigned_compare.add_gate(and_table, tmp, intermediate1[i]));
    }

    tmp = unsigned_compare.add_gate(false_table, 0, 0);
    for(i=0; i<num_bits; i++) {
        tmp = unsigned_compare.add_gate(or_table, intermediate2[i], tmp);
    }

    unsigned_compare.mark_as_output(tmp);

#undef SENDER_BIT
#undef RECVER_BIT
    return unsigned_compare;
}

/******************************************************************************/
/* SenderEvaluator class ******************************************************/
/******************************************************************************/
class SenderEvaluator{
private:
    Circuit ungarbled_circuit;
    Circuit garbled_circuit;
    size_t num_bits;
public:
    SenderEvaluator(size_t num_bits);
    void execute_protocol(int sd);
    void send_garbled_tables(int sd);
    void send_sender_inputs(int sd);
    void send_outputs(int sd);
    void serve_wires(int sd);
};

SenderEvaluator::SenderEvaluator(size_t num_bits) : ungarbled_circuit(num_bits), garbled_circuit(num_bits) {
    this->num_bits = num_bits;
}

void SenderEvaluator::execute_protocol(int sd){
    this->send_garbled_tables(sd);
    this->send_sender_inputs(sd);
    this->send_outputs(sd);
    this->serve_wires(sd);
}

void SenderEvaluator::send_garbled_tables(int sd){
    cout << "I'M SENDING GARBLED TABLES NOW " << sd << endl;
}

void SenderEvaluator::send_sender_inputs(int sd){
    cout << "I'M SENDING MY INPUTS NOW " << sd << endl;
}

void SenderEvaluator::send_outputs(int sd){
    cout << "I'M SENDING OUTPUTS NOW " << sd << endl;
}

void SenderEvaluator::serve_wires(int sd){
    cout << "I'M SERVING WIRES NOW " << sd << endl;
}

/******************************************************************************/
/* ReceiverEvaluator class ****************************************************/
/******************************************************************************/
class ReceiverEvaluator{
private:
    Circuit garbled_circuit;
public:
    ReceiverEvaluator(size_t num_bits);
    void execute_protocol(int sd);
    void receive_garbled_tables(int sd);
    void receive_sender_inputs(int sd);
    void receive_outputs(int sd);
    void request_wires(int sd, int index, int bit);
};

ReceiverEvaluator::ReceiverEvaluator(size_t num_bits) : garbled_circuit(num_bits) {
}

void ReceiverEvaluator::execute_protocol(int sd){
    this->receive_garbled_tables(sd);
    this->receive_sender_inputs(sd);
    this->receive_outputs(sd);
    this->request_wires(sd, 0, 0);
}

void ReceiverEvaluator::receive_garbled_tables(int sd){
    cout << "I'M RECEIVING GARBLED TABLES NOW " << sd << endl;
}

void ReceiverEvaluator::receive_sender_inputs(int sd){
    cout << "I'M RECEIVING INPUTS NOW " << sd << endl;
}

void ReceiverEvaluator::receive_outputs(int sd){
    cout << "I'M RECEIVING OUTPUTS NOW " << sd << endl;
}

void ReceiverEvaluator::request_wires(int sd, int index, int bit){
    cout << "I'M REQUESTING WIRES NOW " << sd << " " << index << " " << bit << endl;
}

/******************************************************************************/
/* Main ***********************************************************************/
/******************************************************************************/
int main(int argc, char** argv) {
    auto print_sender_usage = [&]() {
        cout << "Usage is " << argv[0] << " sender <hostname> <port> <wealth>" << endl;
    };
    auto print_receiver_usage = [&]() {
        cout << "Usage is " << argv[0] << " receiver <port> <wealth>" << endl;
    };

    //Argument verification and parsing
    if(argc != 4 && argc != 5){
        print_sender_usage();
        cout << "OR" << endl;
        print_receiver_usage();
        return 1;
    }
    if(argc == 5 && strcmp("sender", argv[1]) != 0){
        print_sender_usage();
        return 1;
    }
    if(argc == 4 && strcmp("receiver", argv[1]) != 0){
        print_receiver_usage();
        return 1;
    }

    string hostname;
    unsigned short port;
    uint64_t wealth;

    if(argc == 4){
        port = (unsigned short) atoi(argv[2]);
        wealth = (uint64_t) atoi(argv[3]);
    }
    else{
        hostname = argv[2];
        port = (unsigned short) atoi(argv[3]);
        wealth = (uint64_t) atoi(argv[4]);
    }

    (void)hostname; (void)port; (void)wealth; // suppress -Wunused-but-set-variable

    printf("TODO: everything\n");
    return 0;
}
