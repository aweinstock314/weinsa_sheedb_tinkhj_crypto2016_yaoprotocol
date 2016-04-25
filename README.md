Millionare Problem Using Yao's Garbled Circuit
=======
Authors
-----------
* Jassiem Ifill
* Brian Sheedy
* Jacob Tinkhouser
* Avi Weinstock

Dependencies
-----------
* Boost (God have mercy on you. Avi used Boost variants)
* Crypto++
* g++ 4.8

Circuit
-----------
In plain english, the ith bit of the input values a and b is evaluated using: (a_i < b_i) ^ all previous bits were equal.
This is implemented using xnor, and, or, and "less than" (equivalent to (a_i ^ !b_i)) gates. Each gate's truth table is held
in a vector of gates, and with each gate having indices for its left and right inputs.

Oblivious Transfer
-----------
The oblivious transfer implementation is the same as the RSA 1-2 OT implementation from 
[Wikipedia](https://en.wikipedia.org/wiki/Oblivious_transfer#1-2_oblivious_transfer).

Limitations
-----------
* Only supports comparison of unsigned integers that can be represented using 64 bits.
* Garbled evaluator only supports topologically sorted circuits
