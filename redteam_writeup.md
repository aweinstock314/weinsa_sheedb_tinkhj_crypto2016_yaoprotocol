# Red team writeup
Authors
-----------
* Jassiem Ifill
* Brian Sheedy
* Jacob Tinkhouser
* Avi Weinstock

## General techniques

- `strace -ttt foo 2> foo.log` will print high-resolution timing information for all syscalls to `foo.log`  
use this as a lightweight framework for timing attacks

## Projects
### `AlanJamesMike_yao.zip`
- Suspicious things:
    - `yao_server.cpp`: `while( " 'Unnoticeable backdoors are hard to make, props to the nsa' - Mike Macelletti, 2016" ){`  
    (a string literal counts as nonzero, so this is an infinite loop, equivalent to `while(1){`)
    - `circuit.cpp`:
        - `Gate::Gate()` does not initialize the keys to random, it leaves them uninitialized
        - The `for(...) { switch(i) { ... } }` in `Circuit::generateBitComparator(unsigned)` to deduplicate code

While the protocol itself appears to be secure and not leak any information, the implementation itself has a potential information leak. The implementation allows multiple connections to the server instead of only allowing one connection per run. This would allow an adversary to connect multiple times with different values in order to perform a binary search on the server's input. This is possible on all implementations if both parties agree to run the protocol multiple times, and one always inputs the same value, but it is easier in this case since the multiple runs of the protocol are performed on what is known to be the same value. This is not a semi-honest attack, though, as the adversary does not input their real value.

The implementation appears to be noticeably faster than ours, as it takes approximately 5 seconds to complete compared to our 6.25 seconds. This is with the number of bits specified as 64, so the running times should be directly comparable. This is likely due to a faster oblivious transfer implementation, as that is where the majority of time is spent in our implementation.

The client has some memory leaks, but no use of uninitialized memory. The valgrind output for the server is not useful, as there is no way to end the process without killing it prematurely, but it is likely that there exist some memory leaks in there, as well. However, these memory leaks are not useful for obtaining information, so they do not affect the security of the protocol.

#### Client
```
==6746== HEAP SUMMARY:
==6746==     in use at exit: 225,440 bytes in 4,233 blocks
==6746==   total heap usage: 1,113,167 allocs, 1,108,934 frees, 133,965,086 bytes allocated
==6746== 
==6746== LEAK SUMMARY:
==6746==    definitely lost: 45,472 bytes in 393 blocks
==6746==    indirectly lost: 179,968 bytes in 3,840 blocks
==6746==      possibly lost: 0 bytes in 0 blocks
==6746==    still reachable: 0 bytes in 0 blocks
==6746==         suppressed: 0 bytes in 0 blocks
==6746== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

### `brenoc_crypto_MPC_project_v1.zip`
- There's a neatly organized filesystem hierarchy:
    - `client` contains the receiver/circuit evaluator
    - `host` contains the sender/circuit constructor
    - `util` contains abstractions used by both halves
- `{client,host}/main_{client,host}.cc` contain networking code, and calls to `{client,host}/{client,host}_solve.cc`
- a `Connection` abstraction in `util/connection.cc` wraps an fd and provides both normal and oblivious transfer
    - normal transfer (`Connection::send_data`) has a size limit of 2**16, on account of using a 2-byte length prefix
    - normal transfer handles endianness in the size prefix portably  
    (by contrast, our code just blindly shoves the bytes of a `size_t` over the socket)
    - oblivous transfer (`Connection::{send,recv}_2_ot`) fits on a single page due to nice object-oriented RSA abstractions
- `{client,host}_solve.cc` contain the circuit evaluation code
    - `host_solve.cc`'s `permute_table` uses `std::mt19937` to shuffle the tables  
    Mersenne Twister's state can be recovered given 624 consecutive outputs<sup>1</sup>, so this initially looked suspicious  
    it's reseeded from a `std::random_device` every gate though, so 4 outputs will be leaked per seed, so this is safe
    - both files have `solve_problem` with the same signature (`bool solve_problem(std::vector<bool>, Connection)`)
    - the input length is `#define`ed to be 32, and the initialization code looks like it would be correct up to 64
    - `client_solve.cc`'s `solve_problem`:
        1. Receives the sender's garbled input directly
        2. Receives it's own garbled input via OT
        3. Receives and evaluates the circuit one gate at a time
            - There are 5 tiers of gates: {`lt`, `eq`, `cum_eq`, `cmp`, `cum_cmp`}
            - These appear to be short for: {`LessThan`, `Equal`, `CumulativeEqual`, `Comparison`, `CumulativeComparison`}
            - The circuit is not well-abstracted (unlike the networking code): it's all inlined into `solve_problem`
        4. Sends the final garbled value to the sender
        5. Returns the answer based on the sender's response (i.e. the sender provides the output mapping)
    - `host_solve.cc`'s `solve_problem`:
        1. Generates keys for input wires (16 bytes each) using `util/DataStream.cc`'s `get_random_key`  
        `get_random_key` internally uses `/dev/urandom`, so it's good-quality randomness
        2. Sends the keys corresponding to its own input
        3. Sends the receiver's keys via OT
        4. Generates and sends each gate individually, using 12-byte keys with 4-bytes of zero-padding
            - truth tables and parent wire indices are hard-coded into each gate's generation code
        5. Receives the key corresponding to the output wire
        6. Sends the value of the output wire

### `EthanJoshuaChris.zip`
We were unable to find any backdoors or other ways to obtain more information than the protocol specifies. As far as we can tell, the protocol performs exactly as it should in a reasonable time. In fact, the protocol is faster than our code. However, we suspect this is due to the fact that the our code operates on 64-bit integers, while theirs operates on 32-bit integers. Their code takes approximately 4.5 seconds to complete, while ours completes in approximately 6.25 seconds, which is less than twice the time despite operating on twice the bits.

Although we were unable to obtain additional information, we did find a number of oddities with the code. First, it was possible for the binaries to segfault at random on inputs that worked previously. Once this occured, the program would continue to segfault when run until recompiled. This seemed very suspicious to us, as different runs of the protocol should be independent. The fact that the program continued to segfault after a single segfault implied that information was being stored and used between multiple runs. However, we were unable to find any sort of backdoor in the code that worked in this way.

Another possible reason for this behavior is memory corruption issues. This is possible, as valgrind shows that the receiver (alice) has numerous memory leaks and uses of uninitialized memory. However, this likely would not be usable in a semi-honest attack. Valgrind output is:

```
==6002== HEAP SUMMARY:
==6002==     in use at exit: 12,348 bytes in 378 blocks
==6002==   total heap usage: 1,276,518 allocs, 1,276,140 frees, 72,256,485 bytes allocated
==6002== 
==6002== LEAK SUMMARY:
==6002==    definitely lost: 2,016 bytes in 126 blocks
==6002==    indirectly lost: 10,332 bytes in 252 blocks
==6002==      possibly lost: 0 bytes in 0 blocks
==6002==    still reachable: 0 bytes in 0 blocks
==6002==         suppressed: 0 bytes in 0 blocks
==6002== ERROR SUMMARY: 17792 errors from 41 contexts (suppressed: 0 from 0)
```

Second, the protocol takes 32-bit signed integers. While the problem is called the "Millionaire's Problem", implying that the numbers being compared are in the millions and thus fit fine in 32-bit signed integers, there exist people in the world with more money than a 32-bit signed integer can store. This means that it's feasible for the protocol to return incorrect outputs on reasonable inputs due to integer overflows.

Finally, the prime used for the finite field as well as the generator used for oblivious transfer are hard-coded instead of being generated at runtime. The prime (we verified that it is in fact prime) used is 133 bits, so it is infeasible to brute force the exponent and break the oblivious transfer implementation. However, it would be more secure if the prime and generator were chosen randomly every run. The fact that they're hard coded to those specific values is very, very suspicious, but again, we could not find any backdoors based on this.

## Footnotes
1. [https://adriftwith.me/coding/2010/08/13/reversing-the-mersenne-twister-rng-temper-function/]()
