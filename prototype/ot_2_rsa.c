//ifillj

#include <iostream>
//#include <crypto++.h>
//#include <cryptopp.h>
//#include <cryptlib.h>
//#include <crptopp/rsa.h>
//#include <cryptopp/cryptlib.h> //For RandomNumberGenerator
#include <cryptopp/rsa.h> //<---- Works
#include <cryptopp/osrng.h> //<--- for AutoSeed rng
#include <stdint.h> //uint8_t ?
#include <cstdio>
using namespace std;
using namespace CryptoPP;

//Compile with
//g++ ot_2_rsa.c -lcryptopp

typedef std::vector<uint8_t> bytevector;
typedef std::vector<bool> bitvector;

//void ot_sec(int s_fd, int r_fd);
void test();
void ot_send(int sockfd, bytevector msg );
bytevector ot_recv(int sockfd,  uint8_t bit );

int main(int argc, char * argv[])
{
  test();
  return 0;
}


//Exerpt from config.h
//typedef unsigned char byte;     // put in global namespace to avoid ambiguity with other byte typedefs



//To transmit length (for a given message):
  //sender
  //size_t len = std::min(x.size(), y.size());
  // write_aon(fd, (char*)&len, sizeof(size_t));
  //receiver 
  //size_t len;
  // read_aon(fd, (char*)&len, sizeof(size_t));

void test()
{

  //method 1
  //InvertibleRSAFunction params;

  //method 2
  AutoSeededRandomPool prng;
  RSA::PrivateKey private_key;

  //GenerateRandomWithKeySize(<prng>,<key size>)
  //private_key.GenerateRandomWithKeySize(prng,256);
  //private_key.GenerateRandomWithKeySize(prng,1536);
  private_key.GenerateRandomWithKeySize(prng,1024);
  RSA::PublicKey public_key(private_key);

    //unsigned int Integer::ByteCount () const
    //Determines the number of bytes required to represent the Integer.

  const Integer  n = private_key.GetModulus();
  const Integer  p = private_key.GetPrime1();
  const Integer  q = private_key.GetPrime2();
  const Integer  d = private_key.GetPrivateExponent();
  const Integer  e = private_key.GetPublicExponent();


   cout << "RSA Parameters :" << endl;
   cout << " n: " << n << endl;
   cout << " p: " << p << endl;
   cout << " q: " << q << endl;
   cout << " d: " << d << endl;
   cout << " e: " << e << endl;
   cout << endl;
   fprintf(stderr,"n has %i bytes | %i bits\n\n",n.ByteCount(),n.BitCount());
   unsigned char * buff_n = (unsigned char *)calloc(n.ByteCount(),sizeof(unsigned char));
   n.Encode(buff_n,n.ByteCount(),Integer::UNSIGNED);
   //char 
   unsigned char *t;
   for(t = buff_n;t< buff_n + n.ByteCount();t+= sizeof(unsigned char))
     fprintf(stderr,"%c",*t);
   fprintf(stderr,"\n");
   Integer f;
   f.Decode(buff_n,n.ByteCount(),Integer::UNSIGNED);
   cout<<"The decoded integer is: "<< f<<endl;

   free(buff_n);
   char * buff_2 = "This is a message";
   int len_2 = strnlen(buff_2,100);
   Integer buff_int_2 = Integer(buff_2);
   Integer rand_int = Integer(prng,len_2);
   Integer rand_int_1024 = Integer(prng,1024);
   cout<<"(Rand) Integer is: " << rand_int << endl;
   cout<<"\n(Rand 1024) Integer is: "<<rand_int_1024<<endl;


   //self encryption and decryption (works in this case) 
   //(comment stack overflow remarks that this doesn't pad, and may be a security vulnerability)
   //http://stackoverflow.com/questions/14783805/decrypting-data-with-rsa-private-key-function (by jww)
   Integer encrypted_rand = a_exp_b_mod_c(rand_int,d,n);
   cout<<"(Encrypted Rand) Integer is: " << encrypted_rand << endl;
   Integer decrypted_rand = a_exp_b_mod_c(encrypted_rand,e,n);
   cout<<"(Decrypted Rand) Integer is: " << decrypted_rand << endl;

   //through abstraction

   Integer ct = public_key.ApplyFunction(rand_int);
   Integer pt = private_key.CalculateInverse(prng,ct);
   cout<<"\n(Encrypted Rand)[api] Integer is: " << ct << endl;
   cout<<"(Decrypted Rand)[api] Integer is: " << pt << endl;

   //RSA with OAEP & SHA (and filters w/ cryptopp pipelining)
   //encrypt
   RSAES_OAEP_SHA_Encryptor encryptor(public_key);
   string plain_text(buff_2), cipher_text,recovered_text;
   StringSource s_s1(plain_text, true, new PK_EncryptorFilter(prng,encryptor, new StringSink (cipher_text) ));
   //decrypt
   RSAES_OAEP_SHA_Decryptor decryptor(private_key);

   StringSource s_s2(cipher_text,true, new PK_DecryptorFilter(prng, decryptor, new  StringSink (recovered_text)));

   cout<<"\n(Plaintext)[api] [rsaes_oaep_sha] str is: " << plain_text << endl;
   cout<<"(Ciphertext)[api] [rsaes_oaep_sha] str is: " << cipher_text << endl;
   cout<<"(Decrypted plaintext)[api] [rsaes_oaep_sha] str is: " << recovered_text << endl;

   
   

   // #include "nbtheory.h"
   //a_exp_b_mod_c()

   //byte foo;
   //Integer::UNSIGNED
  

   //void Integer::Encode (byte * output,
  //size_t 	outputLen,
  //Signedness 	sign = UNSIGNED 
  //)		const
  //Encode in big-endian format.

  //Parameters
  //output	big-endian byte array
  //outputLen	length of the byte array
  //sign	enumeration indicating Signedness
  //Unsigned means encode absolute value, signed means encode two's complement if negative.
  //outputLen can be used to ensure an Integer is encoded to an exact size (rather than a minimum size). An exact size is useful, for example, when encoding to a field element size


  //ConvertToLong()	
  /*
   cout << "RSA Parameters (converted to long):" << endl;
   cout << " n: " << (unsigned long)n.ConvertToLong() << endl;
   cout << " p: " << p.ConvertToLong() << endl;
   cout << " q: " << q.ConvertToLong() << endl;
   cout << " d: " << d.ConvertToLong() << endl;
   cout << " e: " << e.ConvertToLong() << endl;
  cout << endl;
  */

  /*
  //(Example code)
  // Generate Parameters
  InvertibleRSAFunction params;
  params.GenerateRandomWithKeySize(prng, 64);

  // Generated Parameters
  const Integer& n = params.GetModulus();
  const Integer& p = params.GetPrime1();
  const Integer& q = params.GetPrime2();
  const Integer& d = params.GetPrivateExponent();
  const Integer& e = params.GetPublicExponent();


  cout << "RSA Parameters:" << endl;
  cout << " n: " << n << endl;
  cout << " p: " << p << endl;
  cout << " q: " << q << endl;
  cout << " d: " << d << endl;
  cout << " e: " << e << endl;
  cout << endl;
  */


    //unsigned int Integer::BitCount ()	const
    //Determines the number of bits required to represent the Integer.
    //Returns
    //number of significant bits = floor(log2(abs(*this))) + 1

    /*
      void GeneratableCryptoMaterial::GenerateRandomWithKeySize	( RandomNumberGenerator & rng, unsigned int keySize )		
      Generate a random key or crypto parameters.

      Parameters
       - rng	a RandomNumberGenerator to produce keying material
       - keySize	the size of the key, in bits
       Exceptions
       - KeyingErr	if a key can't be generated or algorithm parameters are invalid
       GenerateRandomWithKeySize calls GenerateRandom with a NameValuePairs object with only "KeySize"
    */
}


void ot_send(int sockfd,  bytevector msg1, bytevector msg2 ) //sender's interface to our 1-2 OST
{

 // Step 1 
   // Do nothing

  /*
    Step 2
   - S generates an RSA key pair
   - Key has the modulus N, private expponent d, public expponent e
   - public key = {e,N}, private key = {e,d,N}
   */



  /*
  Step 3
  - S generates two random values, r_0, r_1
  - S sends e
  - S sends N

  - S sends r_0
  - S sends r_1
  */

  //Step 4
  // Do nothing
  


  /*
    Step 5
    - S receives v from R
    - S applies both r_0, and r_1 to v, to come up with two possible values for rk

   */
  //Step 6
  //Do nothing

  /*
    Step 7
    - S combines the two secret messages with each possible value for rk
    - S produces c_0 = m_0 + rk_0, and c_1 = m_1 + rk_1
    - S sends c_0, and c_1 to S
   */
  //Step 8
   //Do nothing

}
bytevector ot_recv(int sockfd, uint8_t bit) //
{

  //Step 1-2
  /*Step 3
    - recv e
    - recv N
    - recv r_0
    - recv r_1    
   */
  //Step 4
  //Choose bit b (either 0 or 1)

  /*
    Step 5
    - Generate random value, rk
    - Use random value rk, to blind r_b, by computing v = ((r_b  + (rk^e)) mod N)
    - Send v
   */

  //Step 6
  // - Do nothing
  /*
    Step 7
    - Receive c_0,c_1
   */

  /*Step 8
     Get message from ciphertext, mb = cb - rk
   */
}


//Note
// try using length prefixes to signify the length of the message for OT
// implementing 1-2 OST (Oblivious string transfer), as opposed to 1-2 OT, using bit vectors


/*

// S: m_0, m_1
// R: bit b (b indicates desired message)

//Result S doesn't learn b, R learns m_b and nothing else

+--------+
| Step 1 |
+--------+

- (Initial state of 1-2 OT)
+--------+
| Step 2 |
+--------+

- S generates an RSA key pair
  - Key has the modulus N, private expponent d, public expponent e

+--------+
| Step 3 |
+--------+ 

- S generates two random values, r_0, r_1
- S sends r_0, r_1, e,N to R

-  e, N (sent on socket), (length prefix or pad to fixed max size?)
- r_0, r_1 (send on socket), (length prefix or pad to fixed max size?)

+--------+
| Step 4 |
+--------+ 

- R picks b to be either 0, or 1, and chooses r_b

- (Nothing is sent)
+--------+
| Step 5 |
+--------+  

- R generates a random value rk (not a single bit!)
- R uses rk to blind his choice of r_b by computing,
    v = ((r_b  + (rk^e)) mod N)

 - R sends v to S

- v can be sent on socket, (length prefix or pad to fixed max size?)


+--------+
| Step 6 |
+--------+  

- S applies both r_0, and r_1 to v, to come up with two possible values for rk

- The possible values R has are rk_0 = ((v - r_0) ^d mod N)
    -- and rk_1 = ((v - r_1)^d  mod N).

- (One of rk_0, or rk_1 will be able to be decrypted by bob (as one of them will be equal to rk), and the other will be useless  )

+--------+
| Step 7 |
+--------+  

- S combines the two secret messages with each possible value for rk

- S produces c_0 = m_0 + rk_0, and c_1 = m_1 + rk_1

- S sends c_0, and c_1 to R

+--------+
| Step 8 |
+--------+  

- R already knows his choice of b from step 4, so R only decrypts mb = cb - rk, and recieves his message.


 */
