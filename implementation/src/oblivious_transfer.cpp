//Oblivious transfer based off the RSA implementation on Wikipedia

#include <iostream>
//#include <cryptopp.h>
//#include <cryptopp.h>
//#include <cryptlib.h>
//#include <crptopp/rsa.h>
//#include <cryptopp/cryptlib.h> //For RandomNumberGenerator
#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <cryptopp/modarith.h>

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


void RSAObliviousTransfer::send(int sockfd,  bytevector msg1, bytevector msg2 ){
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
    //cout << "Sent random message 1 " << rand_msg_1 << endl;
    //cout << "Sent random message 2 " << rand_msg_2 << endl;

    #ifdef DBG_OT
    buf_size = d.ByteCount();
    write_aon(sockfd, (char*)&buf_size, sizeof(buf_size));
    buffer = new byte[buf_size];
    d.Encode(buffer, buf_size);
    write_aon(sockfd, (char*)buffer, buf_size);
    delete[] buffer;
    #endif

    //Receive receiver's blinded encryption
    rc = read_aon(sockfd, (char*)&buf_size, sizeof(buf_size));
    CHECK_RW(rc, sizeof(buf_size), "Failed to read all bytes from blinded encryption size")
    buffer = new byte[buf_size];
    rc = read_aon(sockfd, (char*)buffer, buf_size);
    CHECK_RW(rc, buf_size, "Failed to read all bytes from blinded encryption")
    Integer v = Integer(buffer, buf_size);
    delete[] buffer;

    //Calculate k's
    //ModularArithmetic ma(n);
    Integer k0 = a_exp_b_mod_c((v - rand_msg_1), d, n);
    Integer k1 = a_exp_b_mod_c((v - rand_msg_2), d, n);
    //Integer k0 = ma.Exponentiate((v - rand_msg_1), d);
    //Integer k1 = ma.Exponentiate((v - rand_msg_2), d);
    /*cout << "Sent modulus " << n << endl;
    cout << "Sent exponent " << e << endl;
    cout << "Using private exponent " << d << endl;
    cout << "Received blinded value " << v << endl;
    cout << "Computed value k0 " << k0 << endl;
    cout << "Computed value k1 " << k1 << endl;*/

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
    //cout << "Sent m0' " << m0_prime << endl;
    //cout << "Sent m1' " << m1_prime << endl;

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

#ifdef DBG_OT
    unsigned int i;
    fprintf(stderr,"\n[ot_send([...])]Message 1 is: ");
    for(i = 0;i<msg1.size();i++)
      fprintf(stderr,"0x%02x ",msg1[i]);
    fprintf(stderr,"\n[ot_send([...])]Message 2 is: ");
    for(i = 0;i<msg2.size();i++)
      fprintf(stderr,"0x%02x %s",msg2[i],(i == (msg2.size() -1))?"\n":"");
#endif
}

bytevector RSAObliviousTransfer::recv(int sockfd, uint8_t bit){
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


    //e_int.Decode(e_buffer,e_size,Integer::UNSIGNED);
        #ifdef DBG_OT
    //fprintf(stderr,"[ot_recv([...])]status: begin step  1\n");
    #endif    
        #ifdef DBG_OT
    //fprintf(stderr,"[ot_recv([...])]status: begin step  2\n");
    #endif    

    //Step 3
#ifdef DBG_OT
    //fprintf(stderr,"[ot_recv([...])]status: begin step  3\n");
    #endif    
    //receive size for N
    read_aon(sockfd, (char*)&n_size, sizeof(unsigned int));
    n_buffer = (byte*)calloc_wrapper(n_size, sizeof(byte));
    //receive contents of N
    read_aon(sockfd, (char*)n_buffer, n_size);
    n_int.Decode(n_buffer, n_size, Integer::UNSIGNED);
    //recieve size for e
    read_aon(sockfd, (char*)&e_size, sizeof(unsigned int));
    e_buffer = (byte *)calloc_wrapper(e_size, sizeof(byte));
    //receive the contents of e
    read_aon(sockfd, (char*)e_buffer, e_size);
    e_int.Decode(e_buffer, e_size, Integer::UNSIGNED);
    //f.Decode(buff_n,n.ByteCount(),Integer::UNSIGNED);

    //receive the size of r_0
    read_aon(sockfd, (char*)&r0_size, sizeof(unsigned int));
    r0_buffer = (byte *)calloc_wrapper(r0_size, sizeof(byte));
    //receive the contents of r_0
    read_aon(sockfd, (char*)r0_buffer, r0_size);
    r0_int.Decode(r0_buffer, r0_size, Integer::UNSIGNED);

    //receive the size of r_1
    read_aon(sockfd, (char*)&r1_size, sizeof(unsigned int));
    r1_buffer = (byte *)calloc_wrapper(r1_size, sizeof(byte));
    //receive the contents of r_1
    read_aon(sockfd, (char*)r1_buffer, r1_size);
    r1_int.Decode(r1_buffer, r1_size, Integer::UNSIGNED);

    //Step 4
    //bit given as argument
        #ifdef DBG_OT
    //fprintf(stderr,"[ot_recv([...])]status: begin step  4\n");
    #endif    
    
    //Step 5
        #ifdef DBG_OT
    //    fprintf(stderr,"[ot_recv([...])]status: begin step  5\n");
    #endif    
    //Integer::Integer	(RandomNumberGenerator & rng,size_t bitCount )	

    Integer random_value( prng, RAND_MESSAGE_SIZE_BITS); //random value of 1024 BITS
    #ifdef DBG_OT
    //random_value = Integer(3);
    #endif

    Integer rb_int = (bit)? r1_int:r0_int; // if b = 0, then  ==> rb = r0; 
    //cout << "Using random value " << random_value << endl;
    //cout << "Using random message " << rb_int << endl;
    

    //a_times_b_mod_c(a,b,c)
    // a_exp_b_mod_c(a,b,c)
    // a_exp_b_mod_c(a,b,c)

    #ifdef DBG_OT
    unsigned int d_size;
    read_aon(sockfd, (char*)&d_size, sizeof(d_size));
    byte* buffer = new byte[d_size];
    read_aon(sockfd, (char*)buffer, d_size);
    Integer d = Integer(buffer, d_size);
    delete[] buffer;
    #endif

    //compute blinded value
    bv_int = (rb_int + a_exp_b_mod_c(random_value, e_int, n_int)) % n_int;
    bv_size = bv_int.ByteCount();    //get size (in bytes)
    //allocate heap block for buffer
    bv_buffer = (byte *) calloc_wrapper(bv_size, sizeof(byte));
    bv_int.Encode(bv_buffer, bv_size);
    //send size of blinded value
    write_aon(sockfd, (char*)&bv_size, sizeof(unsigned int));
    //send contents of blinded value
    write_aon(sockfd, (char*)bv_buffer, bv_size);
    //cout << "Used modulus " << n_int << endl;
    //cout << "Used exponent " << e_int << endl;
    //cout << "Generated k " << random_value << endl;

    #ifdef DBG_OT
    /*cout << "e * d mod n = " << a_times_b_mod_c(e_int, d, n_int) << endl;
    Integer intermediate = bv_int - rb_int;
    Integer calc_k3 = a_exp_b_mod_c(intermediate, d, n_int);
    Integer calc_k = a_exp_b_mod_c((bv_int - rb_int), d, n_int);
    cout << "Calculated k " << calc_k << endl;
    ModularArithmetic ma(n_int);
    Integer calc_k2 = ma.Exponentiate(random_value, d * e_int);
    //Integer k1 = ma.Exponentiate((v - rand_msg_2), d);
    Integer calc_k2 = (bv_int - rb_int);
    Integer val = (bv_int - rb_int);
    for(unsigned int j = 0; j < d; j++){
        calc_k2 *= val;
        calc_k2 %= n_int;
    }
    cout << "Calculated our own way k " << calc_k2 << endl;
    cout << "Intermediate calc " << calc_k3 << endl;*/
    #endif

    //Step 6
    //N/A
        #ifdef DBG_OT
    //    fprintf(stderr,"[ot_recv([...])]status: begin step  6\n");
    #endif    
    //Step 7
        #ifdef DBG_OT
    //    fprintf(stderr,"[ot_recv([...])]status: begin step  7\n");
    #endif
    //receive size of c_0
    read_aon(sockfd, (char*)&c0_size, sizeof(unsigned int));
    c0_buffer = (byte *)calloc_wrapper(c0_size, sizeof(byte));
    unsigned int c_0_actual_size;
    read_aon(sockfd, (char*)&c_0_actual_size, sizeof(c_0_actual_size));
    //receive contents of c_0
    read_aon(sockfd, (char*)c0_buffer, c0_size);
    c0_int.Decode(c0_buffer, c0_size, Integer::UNSIGNED);
    //cout << "Received m0' " << c0_int << endl;


    //receive size of c_1
    read_aon(sockfd, (char*)&c1_size, sizeof(unsigned int));
    c1_buffer = (byte *)calloc_wrapper(c1_size, sizeof(byte));
    unsigned int c_1_actual_size;
    read_aon(sockfd, (char*)&c_1_actual_size, sizeof(c_1_actual_size));
    //receive contents of c_1
    read_aon(sockfd, (char*)c1_buffer, c1_size);
    c1_int.Decode(c1_buffer, c1_size, Integer::UNSIGNED);
    //cout << "Received m1' " << c1_int << endl;

    //step 8    
    //get message
    #ifdef DBG_OT
    //    fprintf(stderr,"[ot_recv([...])]status:begin step 8\n");
      #endif
    //choose message
    mb_int = (bit)?(c1_int - random_value):(c0_int - random_value);
    mb_size = (bit)?(c_1_actual_size):(c_0_actual_size);
    mb_buffer = (byte *)calloc(mb_size, sizeof(byte));
    //change to string
    mb_int.Encode(mb_buffer, mb_size, Integer::UNSIGNED);
    for(i = 0;i<mb_size;i++)
      msg_vector.push_back(mb_buffer[i]);

        #ifdef DBG_OT
    //    fprintf(stderr,"[ot_recv([...])]status: free them all!\n");
    #endif    
        //void *memset(void *s, int c, size_t n);
    memset(n_buffer, 0, n_size);
    free(n_buffer);

    memset(e_buffer, 0, e_size);
    free(e_buffer);

    memset(r0_buffer, 0, r0_size);
    free(r0_buffer);

    memset(r1_buffer, 0, r1_size);
    free(r1_buffer);

    memset(bv_buffer, 0, bv_size);
    free(bv_buffer);

    memset(c0_buffer, 0, c0_size);
    free(c0_buffer);

    memset(c1_buffer, 0, c1_size);
    free(c1_buffer);

    memset(mb_buffer, 0, mb_size);
    free(mb_buffer);



#ifdef DBG_OT
    //fprintf(stderr,"[ot_recv([...])]status:begin step  ")
    fprintf(stderr,"[ot_recv([...])][end] bit = %02x\n",bit);
    fprintf(stderr,"[ot_recv([...])][end] ");
    for(i = 0;i<msg_vector.size();i++)
      fprintf(stderr,"0x%02x %s",msg_vector[i],(i == (msg_vector.size() -1))?"\n":"");
    #endif
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
