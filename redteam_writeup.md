# Red team writeup

## General techniques

- `strace -ttt foo 2> foo.log` will print high-resolution timing information for all syscalls to `foo.log`  
use this as a lightweight framework for timing attacks

## Projects
### `AlanJamesMike_yao.zip`

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


## Footnotes
1. [https://adriftwith.me/coding/2010/08/13/reversing-the-mersenne-twister-rng-temper-function/]()
