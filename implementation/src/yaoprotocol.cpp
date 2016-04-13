#include <boost/utility/binary.hpp>
#include <boost/variant.hpp>
#include <iostream>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
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
    InputWire(size_t i, variant<SenderTag, ReceiverTag> who_) : index(i), who(who_) {}
    size_t index;
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
        wires.push_back(InputWire(i, SenderTag()));
    }
    for(size_t i = 0; i < num_bits; i++) {
        wires.push_back(InputWire(i, ReceiverTag()));
    }
}

bool eval_truthtable(uint8_t table, bool x, bool y) {
    /*
    x y t
    0 0 a LSB
    0 1 b
    1 0 c
    1 1 d
    */
    uint8_t x_ = x ? BOOST_BINARY(1100) : BOOST_BINARY(0011);
    uint8_t y_ = y ? BOOST_BINARY(1010) : BOOST_BINARY(0101);
    return table & x_ & y_;
}

struct eval_wire : public static_visitor<> {
    typedef bool result_type;
    Circuit *c;
    bitvector *result;
    bitvector *x, *y;
    bitvector *tempwires, *evalflags;
    size_t i;
    eval_wire(Circuit* c_, bitvector* res, bitvector* x_, bitvector* y_, bitvector* tw, bitvector* ef, size_t i_) :
        c(c_), result(res), x(x_), y(y_), tempwires(tw), evalflags(ef), i(i_) {}
    void mark_evaluated(size_t j, bool b) {
        (*tempwires)[j] = b;
        (*evalflags)[j] = 1;
    }
    bool recursive_eval(size_t j) {
        size_t tmp = i;
        i = j;
        bool b = (*evalflags)[i] ? (*tempwires)[i] : apply_visitor((*this), c->wires[i]);
        mark_evaluated(j, b);
        i = tmp;
        return b;
    }
    bool operator()(InputWire& w) {
        struct matcher : static_visitor<> {
            typedef bool result_type;
            bitvector *x, *y;
            InputWire &w;
            matcher(bitvector *x_, bitvector *y_, InputWire& w_) : x(x_), y(y_), w(w_) {}
            bool operator()(SenderTag&) {
                return (*x)[w.index];
            }
            bool operator()(ReceiverTag&) {
                return (*y)[w.index];
            }
        } m(x,y,w);
        bool b = apply_visitor(m, w.who);
        mark_evaluated(i, b);
        return b;
    };
    bool operator()(GateWire& w) {
        if((*evalflags)[i]) {
            return (*tempwires)[i];
        } else {
            bool l = recursive_eval(w.l);
            bool r = recursive_eval(w.r);
            bool b = eval_truthtable(w.truth_table, l, r);
            mark_evaluated(i, b);
            return b;
        }
    }
    bool operator()(OutputWire& w) {
        bool b = recursive_eval(w.index);
        result->push_back(b);
        return b;
    }
};
bytevector eval_circuit(Circuit c, bytevector x_, bytevector y_) {
    bitvector result;
    bitvector x, y;
    bitvector tempwires(c.wires.size(), 0);
    bitvector evalflags(c.wires.size(), 0);
    x = unpack_bv(x_);
    y = unpack_bv(y_);
    size_t i;
    for(i=0; i<c.wires.size(); i++) {
        auto ew = eval_wire(&c, &result, &x, &y, &tempwires, &evalflags, i);
        apply_visitor(ew, c.wires[i]);
    }
    return pack_bv(result);
}

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
    uint8_t and_table      = BOOST_BINARY(1000);
    uint8_t false_table    = BOOST_BINARY(0000);
    uint8_t lessthan_table = BOOST_BINARY(0010);
    uint8_t or_table       = BOOST_BINARY(1110);
    uint8_t true_table     = BOOST_BINARY(1111);
    uint8_t xnor_table     = BOOST_BINARY(1001);

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
/* Mains **********************************************************************/
/******************************************************************************/

int sender_main(int, char** argv) {
    string hostname = argv[2];
    unsigned short port = (unsigned short) atoi(argv[2]);
    uint64_t wealth = (uint64_t) atoi(argv[3]);
    (void)hostname; (void)port; (void)wealth; // suppress -Wunused-but-set-variable
    int sd, rc;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_UNSPEC; //IPv4 or IPv6 allowed
    hints.ai_socktype = SOCK_STREAM; //TCP
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    rc = getaddrinfo(argv[2], argv[3], &hints, &result);
    if(rc != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return EXIT_FAILURE;
    }

    //Try connecting to each address
    for(rp = result; rp != NULL; rp = rp->ai_next){
        sd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(sd == -1){
            continue;
        }
        if(connect(sd, rp->ai_addr, rp->ai_addrlen) != -1){
            break;
        }
        close(sd);
    }

    if(rp == NULL){
        cerr << "Could not connect to specified host and port" << endl;
        return EXIT_FAILURE;
    }

    freeaddrinfo(result);

    SenderEvaluator eval = SenderEvaluator(sizeof(wealth) * 8);
    eval.execute_protocol(sd);
    return 0;
}

int receiver_main(int, char** argv) {
    unsigned short port = (unsigned short) atoi(argv[2]);
    uint64_t wealth = (uint64_t) atoi(argv[3]);
    (void)port; (void)wealth; // suppress -Wunused-but-set-variable
    int sd, rc;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));

    int server_sd;
    hints.ai_family = AF_UNSPEC; //IPv4 or IPv6 allowed
    hints.ai_socktype = SOCK_STREAM; //TCP
    hints.ai_flags = AI_PASSIVE; //Any IP
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    rc = getaddrinfo(NULL, argv[2], &hints, &result);
    if(rc != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return EXIT_FAILURE;
    }

    //Try to bind to each address
    for(rp = result; rp != NULL; rp = rp->ai_next){
        server_sd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(server_sd == -1){
            continue;
        }
        if(bind(server_sd, rp->ai_addr, rp->ai_addrlen) == 0){
            break;
        }
        close(server_sd);
    }

    if(rp == NULL){
        cerr << "Could not bind to specified port" << endl;
        return EXIT_FAILURE;
    }

    freeaddrinfo(result);

    rc = listen(server_sd, 1);
    struct sockaddr_in client;
    int fromlen = sizeof(client);
    sd = accept(server_sd, (struct sockaddr *) &client, (socklen_t*) &fromlen);
    if(sd < 0){
        perror("Failed to accept connection");
        return EXIT_FAILURE;
    }

    ReceiverEvaluator eval = ReceiverEvaluator(sizeof(wealth) * 8);
    eval.execute_protocol(sd);
    return 0;
}

int main(int argc, char** argv) {
    auto print_sender_usage = [&]() {
        cout << "Usage is " << argv[0] << " sender <hostname> <port> <wealth>" << endl;
    };
    auto print_receiver_usage = [&]() {
        cout << "Usage is " << argv[0] << " receiver <port> <wealth>" << endl;
    };

    auto print_usage = [&]() {
        print_sender_usage();
        cout << "OR" << endl;
        print_receiver_usage();
    };

    //Argument verification and parsing
    if(argc != 4 && argc != 5){
        print_usage();
        return EXIT_FAILURE;
    }
    if(!strcmp("sender", argv[1])) {
        if(argc != 5) {
            print_sender_usage();
            return EXIT_FAILURE;
        } else {
            return sender_main(argc, argv);
        }
    }
    if(!strcmp("receiver", argv[1])) {
        if(argc != 4) {
            print_receiver_usage();
            return EXIT_FAILURE;
        } else {
            return receiver_main(argc, argv);
        }
    }

    print_usage();
    return EXIT_FAILURE;
}
