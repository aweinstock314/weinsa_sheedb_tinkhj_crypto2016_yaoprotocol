#include <boost/variant.hpp>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include "utils.h"

using namespace std;

typedef std::vector<uint8_t> bytevector;

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
/* Circuit class **************************************************************/
/******************************************************************************/
class Circuit{
private:
    int num_bits;
public:
    Circuit(int num_bits);
};

Circuit::Circuit(int num_bits){
    this->num_bits = num_bits;
}

/******************************************************************************/
/* SenderEvaluator class ******************************************************/
/******************************************************************************/
class SenderEvaluator{
private:
    Circuit ungarbled_circuit;
    Circuit garbled_circuit;
    int num_bits;
public:
    SenderEvaluator(int num_bits);
    void execute_protocol(int sd);
    void send_garbled_tables(int sd);
    void send_sender_inputs(int sd);
    void send_outputs(int sd);
    void serve_wires(int sd);
};

SenderEvaluator::SenderEvaluator(int num_bits) : ungarbled_circuit(num_bits), garbled_circuit(num_bits) {
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
    ReceiverEvaluator(int num_bits);
    void execute_protocol(int sd);
    void receive_garbled_tables(int sd);
    void receive_sender_inputs(int sd);
    void receive_outputs(int sd);
    void request_wires(int sd, int index, int bit);
};

ReceiverEvaluator::ReceiverEvaluator(int num_bits) : garbled_circuit(num_bits) {
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
