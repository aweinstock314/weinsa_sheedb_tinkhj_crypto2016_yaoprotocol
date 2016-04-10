#include <stdint.h>
#include <stdio.h>
#include <vector>

#include "utils.h"

typedef std::vector<uint8_t> bytevector;

// placeholder naive implementation for testing
class TerriblyInsecureObliviousTransfer {
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

int main(int argc, char** argv) {
    // silence -Wunused-parameter for skeleton
    (void)argc; (void)argv;
    printf("TODO: everything\n");
}
