#include <iostream>
//#include <crypto++.h>
//#include <cryptopp.h>
//#include <cryptlib.h>
//#include <crptopp/rsa.h>
//#include <cryptopp/cryptlib.h> //For RandomNumberGenerator
#include <cstdio>

#include "oblivious_transfer.h"

using namespace std;
using namespace CryptoPP;

void ot_send(int sockfd,  bytevector msg1, bytevector msg2 ){
    (void)sockfd; (void)msg1;; (void)msg2;
    //Setup RSA
    AutoSeededRandomPool prng;
    RSA::PrivateKey private_key;

    //GenerateRandomWithKeySize(<prng>,<key size>)
    private_key.GenerateRandomWithKeySize(prng,1024);
    RSA::PublicKey public_key(private_key);

    const Integer  n = private_key.GetModulus();
    const Integer  p = private_key.GetPrime1();
    const Integer  q = private_key.GetPrime2();
    const Integer  d = private_key.GetPrivateExponent();
    const Integer  e = private_key.GetPublicExponent();

    char* modulus_buffer = new char[n.ByteCount()];
    for(unsigned int i = 0; i < n.ByteCount(); i++){
        modulus_buffer[i] = n.GetByte(i);
    }
    delete[] modulus_buffer;
}

bytevector ot_recv(int sockfd, uint8_t bit){
    (void)sockfd; (void)bit;
    return bytevector();
}