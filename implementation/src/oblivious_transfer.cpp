//Oblivious transfer based off the RSA implementation on Wikipedia

#include <iostream>
//#include <crypto++.h>
//#include <cryptopp.h>
//#include <cryptlib.h>
//#include <crptopp/rsa.h>
//#include <cryptopp/cryptlib.h> //For RandomNumberGenerator
#include <cstdio>

#include "oblivious_transfer.h"

#define RAND_MESSAGE_SIZE_BITS 128
#define CHECK_RW(result, expected, message) \
    if(result != expected){ \
        perror(message); \
        exit(EXIT_FAILURE); \
    }

using namespace std;
using namespace CryptoPP;

void ot_send(int sockfd,  bytevector msg1, bytevector msg2 ){
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

    //Send modulus
    unsigned int buf_size = n.ByteCount();
    size_t rc = write_aon(sockfd, (char*)&buf_size, sizeof(buf_size));
    CHECK_RW(rc, sizeof(buf_size), "Failed to write all bytes of modulus size")
    byte* buffer = new byte[buf_size];
    n.Encode(buffer, buf_size);
    rc = write_aon(sockfd, (char*)buffer, buf_size);
    CHECK_RW(rc, buf_size, "Failed to write all bytes of modulus")
    delete[] buffer;

    //Send public exponent
    buf_size = e.ByteCount();
    rc = write_aon(sockfd, (char*)&buf_size, sizeof(buf_size));
    CHECK_RW(rc, sizeof(buf_size), "Failed to write all bytes of exponent size")
    buffer = new byte[buf_size];
    e.Encode(buffer, buf_size);
    rc = write_aon(sockfd, (char*)buffer, buf_size);
    CHECK_RW(rc, buf_size, "Failed to write all bytes of exponent")
    delete[] buffer;

    //Generate two random messages
    Integer rand_msg_1 = Integer();
    Integer rand_msg_2 = Integer();
    rand_msg_1.Randomize(prng, RAND_MESSAGE_SIZE_BITS);
    rand_msg_2.Randomize(prng, RAND_MESSAGE_SIZE_BITS);
    byte* rand_1 = new byte[rand_msg_1.ByteCount()];
    byte* rand_2 = new byte[rand_msg_2.ByteCount()];
    rand_msg_1.Encode(rand_1, rand_msg_1.ByteCount());
    rand_msg_2.Encode(rand_2, rand_msg_2.ByteCount());

    //Send random messages
    buf_size = rand_msg_1.ByteCount();
    rc = write_aon(sockfd, (char*)&buf_size, sizeof(buf_size));
    CHECK_RW(rc, sizeof(buf_size), "Failed to write all bytes of random message 1 size")
    rc = write_aon(sockfd, (char*)rand_1, buf_size);
    CHECK_RW(rc, buf_size, "Failed to write all bytes of random message 1");
    buf_size = rand_msg_2.ByteCount();
    rc = write_aon(sockfd, (char*)&buf_size, sizeof(buf_size));
    CHECK_RW(rc, sizeof(buf_size), "Failed to write all bytes of random message 2 size")
    rc = write_aon(sockfd, (char*)rand_2, buf_size);
    CHECK_RW(rc, buf_size, "Failed to write all bytes of random message 2")
    delete[] rand_1;
    delete[] rand_2;

    //Receive receiver's blinded encryption
    rc = read_aon(sockfd, (char*)&buf_size, sizeof(buf_size));
    CHECK_RW(rc, sizeof(buf_size), "Failed to read all bytes from blinded encryption size")
    buffer = new byte[buf_size];
    rc = read_aon(sockfd, (char*)buffer, buf_size);
    CHECK_RW(rc, buf_size, "Failed to read all bytes from blinded encryption")
    Integer v = Integer(buffer, buf_size);
    delete[] buffer;

    //Calculate k's
    Integer k0 = a_exp_b_mod_c((v - rand_msg_1), d, n);
    Integer k1 = a_exp_b_mod_c((v - rand_msg_2), d, n);

    //TODO: If weird errors show up during OT, check here for problems like endianness
    //Convert messages to Integers and do math
    Integer m0 = Integer((byte*)&msg1[0], msg1.size());
    Integer m1 = Integer((byte*)&msg2[0], msg2.size());

    Integer m0_prime = Integer();
    Integer m1_prime = Integer();

    m0_prime = m0 + k0;
    m1_prime = m1 + k1;

    //Send messages
    buf_size = m0_prime.ByteCount();
    buffer = new byte[buf_size];
    m0_prime.Encode(buffer, buf_size);
    rc = write_aon(sockfd, (char*)&buf_size, sizeof(buf_size));
    CHECK_RW(rc, sizeof(buf_size), "Failed to write all bytes of m0' size")
    rc = write_aon(sockfd, (char*)buffer, buf_size);
    CHECK_RW(rc, buf_size, "Failed to write all bytes of m0'")
    delete[] buffer;

    buf_size = m1_prime.ByteCount();
    buffer = new byte[buf_size];
    m1_prime.Encode(buffer, buf_size);
    rc = write_aon(sockfd, (char*)&buf_size, sizeof(buf_size));
    CHECK_RW(rc, sizeof(buf_size), "Failed to write all bytes of m1' size")
    rc = write_aon(sockfd, (char*)buffer, buf_size);
    CHECK_RW(rc, buf_size, "Failed to write all bytes of m1'")
    delete[] buffer;
}

bytevector ot_recv(int sockfd, uint8_t bit){
    (void)sockfd; (void)bit;
    return bytevector();
}