#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <string>

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

using namespace std;

/******************************************************************************/
/* Circuit class **************************************************************/
/******************************************************************************/
class Circuit{
private:
	int num_bits;
public:
	Circuit(int num_bits);
};

Circuit::Circuit(int num_bits){
	this.num_bits = num_bits;
}

/******************************************************************************/
/* SenderEvaluator class ******************************************************/
/******************************************************************************/
class SenderEvaluator{
private:
	Circuit ungarbled_circuit;
	Circuit garbled_circuit;
public:
	SenderEvaluator(int num_bits);
};

int main(int argc, char** argv) {
    // silence -Wunused-parameter for skeleton
    (void)argc; (void)argv;

    //Argument verification and parsing
    if(argc != 4 && argc != 5){
    	cout << "Usage is " << argv[0] << "sender <hostname> <port> <wealth>" << endl;
    	cout << "OR" << endl;
    	cout << "Usage is " << argv[0] << "receiver <port> <wealth>" << endl;
    	return 1;
    }
    if(argc == 5 && strcmp("sender", argv[1]) != 0){
    	cout << "Usage is " << argv[0] << "sender <hostname> <port> <wealth>" << endl;
    	return 1;
    }
    if(argc == 4 && strcmp("receiver", argv[1]) != 0){
    	cout << "Usage is " << argv[0] << "receiver <port> <wealth>" << endl;
    	return 1;
    }

    string hostname;
    unsigned short port;
    uint64_t wealth;

    if(argc == 4){
    	port = (unsigned short) atoi(argv[2]);
    	wealth = (uint64_t) atoi(argv[3]);
    }
    else{
    	hostname = argv[2];
    	port = (unsigned short) atoi(argv[3]);
    	wealth = (uint64_t) atoi(argv[4]);
    }



    printf("TODO: everything\n");
    return 0;
}
