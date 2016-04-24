//Oblivious transfer based off the RSA implementation on Wikipedia

#include <iostream>
//#include <crypto++.h>
//#include <cryptopp.h>
//#include <cryptlib.h>
//#include <crptopp/rsa.h>
//#include <cryptopp/cryptlib.h> //For RandomNumberGenerator
#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>

#include "oblivious_transfer.h"

#define RAND_MESSAGE_SIZE_BITS 128
#define CHECK_RW(result, expected, message) \
    if(result != expected){ \
        perror(message); \
        exit(EXIT_FAILURE); \
    }

using namespace std;
using namespace CryptoPP;

void * calloc_wrapper(unsigned int num_members,unsigned int num_size);
void * malloc_wrapper(unsigned int n);

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
    unsigned int actual_size = msg1.size();
    buffer = new byte[buf_size];
    m0_prime.Encode(buffer, buf_size);
    rc = write_aon(sockfd, (char*)&buf_size, sizeof(buf_size));
    CHECK_RW(rc, sizeof(buf_size), "Failed to write all bytes of m0' size")
    //Need to send original bytevector size in order to prevent cases of leading 0 truncation
    rc = write_aon(sockfd, (char*)&actual_size, sizeof(actual_size));
    CHECK_RW(rc, sizeof(actual_size), "Failed to write all bytes of actual m0 size")
    rc = write_aon(sockfd, (char*)buffer, buf_size);
    CHECK_RW(rc, buf_size, "Failed to write all bytes of m0'")
    delete[] buffer;

    buf_size = m1_prime.ByteCount();
    actual_size = msg2.size();
    buffer = new byte[buf_size];
    m1_prime.Encode(buffer, buf_size);
    rc = write_aon(sockfd, (char*)&buf_size, sizeof(buf_size));
    CHECK_RW(rc, sizeof(buf_size), "Failed to write all bytes of m1' size")
    rc = write_aon(sockfd, (char*)&actual_size, sizeof(actual_size));
    CHECK_RW(rc, sizeof(actual_size), "Failed to write all bytes of actual m1 size")
    rc = write_aon(sockfd, (char*)buffer, buf_size);
    CHECK_RW(rc, buf_size, "Failed to write all bytes of m1'")
    delete[] buffer;
}

bytevector ot_recv(int sockfd, uint8_t bit){
  //(void)sockfd; (void)bit;


    unsigned int e_size,n_size,r0_size,r1_size;
    unsigned int bv_size,c0_size,c1_size;
    unsigned int mb_size;
    unsigned int i;
    byte * e_buffer;
    byte *n_buffer;
    byte *r0_buffer;
    byte *r1_buffer;
    byte * bv_buffer;
    byte *c0_buffer;
    byte *c1_buffer;
    byte * mb_buffer;
    Integer e_int, n_int,r0_int, r1_int;
    Integer bv_int,c0_int,c1_int,cb_int,mb_int;
    AutoSeededRandomPool prng;
    bytevector msg_vector;
    //ssize_t recv(int sockfd, void *buf, size_t len, int flags);

    //void Integer::Decode(const byte * input,size_t inputLen, Signedness sign = UNSIGNED )	


    //e_int.Decode(e_buffer,e_size,Integer::UNSIGNED);
    //Step 3
    //receive size for N
    recv(sockfd,(void *)&n_size,sizeof(unsigned int),0);
    n_buffer = (byte *)calloc_wrapper(n_size,sizeof(byte));
    //receive contents of N
    recv(sockfd,(void *)n_buffer,n_size,0);
    n_int.Decode(n_buffer,n_size,Integer::UNSIGNED);
    //recieve size for e
    recv(sockfd, (void *)&e_size,sizeof(unsigned int),0);
    e_buffer = (byte *)calloc_wrapper(e_size,sizeof(byte));
    //receive the contents of e
    recv(sockfd,(void *)e_buffer,e_size,0);
    e_int.Decode(e_buffer,e_size,Integer::UNSIGNED);
    //f.Decode(buff_n,n.ByteCount(),Integer::UNSIGNED);

    //receive the size of r_0
    recv(sockfd,(void *)&r0_size,sizeof(unsigned int),0);
    r0_buffer = (byte *)calloc_wrapper(r0_size,sizeof(byte));
    //receive the contents of r_0
    recv(sockfd,(void *)r0_buffer,r0_size,0);
    r0_int.Decode(r0_buffer,r0_size,Integer::UNSIGNED);

    //receive the size of r_1
    recv(sockfd,(void *)&r1_size,sizeof(unsigned int),0);
    r1_buffer = (byte *)calloc_wrapper(r1_size,sizeof(byte));
    //receive the contents of r_1
    recv(sockfd,(void *)r1_buffer,r1_size,0);
    r1_int.Decode(r1_buffer,r1_size,Integer::UNSIGNED);

    //Step 4
    //bit given as argument
    
    //Step 5

    //Integer::Integer	(RandomNumberGenerator & rng,size_t bitCount )	

    Integer random_value( prng, 1024); //random value of 1024 BITS

    Integer rb_int = (bit)? r1_int:r0_int; // if b = 0, then  ==> rb = r0; 
    

    //a_times_b_mod_c(a,b,c)
    // a_exp_b_mod_c(a,b,c)
    // a_exp_b_mod_c(a,b,c)

    //compute blinded value
    bv_int = (rb_int + a_exp_b_mod_c(random_value,e_int,n_int)) % n_int;
    bv_size = bv_int.ByteCount();    //get size (in bytes)
    //allocate heap block for buffer
    bv_buffer = (byte *) calloc_wrapper(bv_size,sizeof(byte));
    bv_int.Encode(bv_buffer, bv_size);
    //send size of blinded value
    send(sockfd,(void *)&bv_size,sizeof(unsigned int),0);
    //send contents of blinded value
    send(sockfd,(void *)bv_buffer,bv_size,0);

    //Step 6
    //N/A
    
    //Step 7
    
    //receive size of c_0
    recv(sockfd,(void *)&c0_size,sizeof(unsigned int),0);
    c0_buffer = (byte *)calloc_wrapper(c0_size,sizeof(byte));
    unsigned int c_0_actual_size;
    recv(sockfd, (void*)&c_0_actual_size, sizeof(c_0_actual_size), 0);
    //receive contents of c_0
    recv(sockfd,(void *)c0_buffer,c0_size,0);
    c0_int.Decode(c0_buffer,c0_size,Integer::UNSIGNED);


    //receive size of c_1
    recv(sockfd,(void *)&c1_size,sizeof(unsigned int),0);
    c1_buffer = (byte *)calloc_wrapper(c1_size,sizeof(byte));
    unsigned int c_1_actual_size;
    recv(sockfd, (void*)&c_1_actual_size, sizeof(c_1_actual_size), 0);
    //receive contents of c_1
    recv(sockfd,(void *)c1_buffer,c1_size,0);
    c1_int.Decode(c1_buffer,c1_size,Integer::UNSIGNED);

    //step 8    
    //get message

    //choose message
    mb_int = (bit)?(c1_int - random_value):(c0_int - random_value);
    mb_size = (bit)?(c_1_actual_size):(c_0_actual_size);
    mb_buffer = (byte *)calloc(mb_size,sizeof(byte));
    //change to string
    mb_int.Encode(mb_buffer,mb_size,Integer::UNSIGNED);
    for(i = 0;i<mb_size;i++)
      msg_vector.push_back(mb_buffer[i]);

    return msg_vector;
}


void * malloc_wrapper(unsigned int n)
{

  void * temp = NULL;
  while( !(temp = malloc(n)));
  return temp;


}

void * calloc_wrapper(unsigned int num_members,unsigned int num_size)
{

  void * temp = NULL;
  while( !(temp = calloc(num_members,num_size)));
  return temp;
}
