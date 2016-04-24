#include "Circuit.h"
#include "utils.h"

Circuit::Circuit(size_t num_bits){
    this->num_bits = num_bits;
    for(size_t i = 0; i < num_bits; i++) {
        wires.push_back(InputWire(i, SenderTag()));
    }
    for(size_t i = 0; i < num_bits; i++) {
        wires.push_back(InputWire(i, ReceiverTag()));
    }
}

size_t Circuit::add_gate(uint8_t truth_table, size_t l, size_t r) {
    wires.push_back(GateWire(truth_table, l, r));
    return wires.size() - 1;
}

void Circuit::mark_as_output(size_t index) {
    wires.push_back(OutputWire(index));
}

variant<SenderTag, ReceiverTag> bool_to_tag(bool b) {
    SenderTag s;
    ReceiverTag r;
    variant<SenderTag, ReceiverTag> v = r;
    if(b) { v = s; }
    return v;
}

bool tag_to_bool(variant<SenderTag, ReceiverTag> b) {
    struct matcher : public boost::static_visitor<> {
        typedef bool result_type;
        bool operator()(SenderTag&) { return true; }
        bool operator()(ReceiverTag&) { return false; }
    } m;
    return boost::apply_visitor(m, b);
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
