#ifndef RECV_EVAL_H
#define RECV_EVAL_H

#include "Circuit.h"

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

#endif