#include <iostream>

#include "ReceiverEvaluator.h"

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