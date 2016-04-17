#ifndef SEND_EVAL_H
#define SEND_EVAL_H

#include "Circuit.h"

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

#endif