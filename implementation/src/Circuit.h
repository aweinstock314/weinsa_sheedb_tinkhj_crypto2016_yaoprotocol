#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <boost/variant.hpp>
#include "utils.h"

using namespace boost;
using namespace std;

/******************************************************************************/
/* Wires and Related Structs **************************************************/
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

struct eval_wire;

/******************************************************************************/
/* Circuit class **************************************************************/
/******************************************************************************/

class Circuit {
public:
    size_t num_bits;
    vector<Wire> wires;

    Circuit(size_t num_bits);
    size_t add_gate(uint8_t truth_table, size_t l, size_t r);
    void mark_as_output(size_t index);
};

/******************************************************************************/
/* Related functions **********************************************************/
/******************************************************************************/

//Evaluates a truth table with inputs x and y
bool eval_truthtable(uint8_t table, bool x, bool y);

//Evaluates an entire circuit with inputs x_ and y_
bytevector eval_circuit(Circuit c, bytevector x_, bytevector y_);

#endif