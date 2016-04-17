#include <iostream>

#include "SenderEvaluator.h"

using namespace std;

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