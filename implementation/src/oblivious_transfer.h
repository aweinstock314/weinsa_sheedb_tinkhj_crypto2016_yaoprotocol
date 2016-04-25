#ifndef OT_H
#define OT_H
#include <cryptopp/rsa.h> //<---- Works
#include <cryptopp/osrng.h> //<--- for AutoSeed rng

#include "utils.h"

class RSAObliviousTransfer{
public:
    static void send(int sockfd,  bytevector msg1, bytevector msg2 );
    static bytevector recv(int sockfd, uint8_t bit);
};
/*void ot_send(int sockfd,  bytevector msg1, bytevector msg2 );
bytevector ot_recv(int sockfd, uint8_t bit);*/

#endif
