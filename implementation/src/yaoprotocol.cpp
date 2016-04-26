#include <bitset>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <vector>

#include "Circuit.h"
#include "GarbledCircuit.h"
#include "oblivious_transfer.h"
#include "ReceiverEvaluator.h"
#include "SenderEvaluator.h"
#include "utils.h"

using namespace boost;
using namespace std;

// placeholder naive implementation for testing
class TerriblyInsecureObliviousTransfer {
    public:
    static void send(int fd, bytevector x, bytevector y) {
        size_t len = std::min(x.size(), y.size());
        write_aon(fd, (char*)&len, sizeof(size_t));
        write_aon(fd, (char*)x.data(), len);
        write_aon(fd, (char*)y.data(), len);
    }
    static bytevector recv(int fd, uint8_t bit) {
        size_t len;
        read_aon(fd, (char*)&len, sizeof(size_t));
        bytevector x(len, 0), y(len, 0);
        read_aon(fd, (char*)x.data(), len);
        read_aon(fd, (char*)y.data(), len);
        return bit ? y : x;
    }
};

/******************************************************************************/
/* The fabled generate_unsigned_compare_circuit function **********************/
/* It generates a circuit that calculates unsigned comparisons ****************/
/******************************************************************************/

// Derived from the following haskell
/*
mkManualLessThan size = output where
    primitive f i = Function f (Input False i) (Input True i)
    lessthans = map (primitive (<)) [size-1,size-2..0]
    xnors = Constant True : map (primitive ((not .) . xor)) [size-1,size-2..0]
    intermediate1 = Constant True : zipWith (Function (&&)) intermediate1 (tail xnors)
    intermediate2 = zipWith3 (\x y z -> (Function (&&) x (Function (&&) y z))) lessthans xnors intermediate1
    output = foldr (Function (||)) (Constant False) intermediate2
*/

//This circuit evaluates a < b, where a is the left input of the circuit and b is the right input
Circuit generate_unsigned_compare_circuit(size_t num_bits) {
    Circuit unsigned_compare { num_bits };
    uint8_t and_table      = BOOST_BINARY(1000);
    uint8_t false_table    = BOOST_BINARY(0000);
    uint8_t lessthan_table = BOOST_BINARY(0010);
    uint8_t or_table       = BOOST_BINARY(1110);
    uint8_t true_table     = BOOST_BINARY(1111);
    uint8_t xnor_table     = BOOST_BINARY(1001);

#define SENDER_BIT(k) (k)
#define RECVER_BIT(k) (num_bits + (k))
    size_t i, tmp;
    vector<size_t> lessthans;
    for(i=num_bits-1; i!=(size_t)-1; i--) {
        lessthans.push_back(unsigned_compare.add_gate(lessthan_table, SENDER_BIT(i), RECVER_BIT(i)));
    }
    vector<size_t> xnors;
    xnors.push_back(unsigned_compare.add_gate(true_table, 0, 0));
    for(i=num_bits-1; i!=(size_t)-1; i--) {
        xnors.push_back(unsigned_compare.add_gate(xnor_table, SENDER_BIT(i), RECVER_BIT(i)));
    }

    vector<size_t> intermediate1;
    intermediate1.push_back(unsigned_compare.add_gate(true_table, 0, 0));
    for(i=1; i<num_bits; i++) {
        intermediate1.push_back(unsigned_compare.add_gate(and_table, intermediate1[i-1], xnors[i]));
    }

    vector<size_t> intermediate2;
    for(i=0; i<num_bits; i++) {
        tmp = unsigned_compare.add_gate(and_table, lessthans[i], xnors[i]);
        intermediate2.push_back(unsigned_compare.add_gate(and_table, tmp, intermediate1[i]));
    }

    tmp = unsigned_compare.add_gate(false_table, 0, 0);
    for(i=0; i<num_bits; i++) {
        tmp = unsigned_compare.add_gate(or_table, intermediate2[i], tmp);
    }

    unsigned_compare.mark_as_output(tmp);

#undef SENDER_BIT
#undef RECVER_BIT
    return unsigned_compare;
}

/******************************************************************************/
/* Mains **********************************************************************/
/******************************************************************************/

bytevector uint64_t_to_bytevector(uint64_t wealth_) {
    bytevector wealth;
    uint8_t* p;
    size_t i;
    for(i=0, p=(uint8_t*)&wealth_; i < sizeof(wealth_); ++i, ++p) {
        wealth.push_back(*p);
    }
    return wealth;
}

int sender_main(int, char** argv) {
    string hostname = argv[2];
    unsigned short port = (unsigned short) atoi(argv[3]);
    uint64_t wealth = (uint64_t) stoull(argv[4]);
    (void)hostname; (void)port; (void)wealth; // suppress -Wunused-but-set-variable
    int sd, rc;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_UNSPEC; //IPv4 or IPv6 allowed
    hints.ai_socktype = SOCK_STREAM; //TCP
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    rc = getaddrinfo(argv[2], argv[3], &hints, &result);
    if(rc != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return EXIT_FAILURE;
    }

    //Try connecting to each address
    for(rp = result; rp != NULL; rp = rp->ai_next){
        sd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(sd == -1){
            continue;
        }
        if(connect(sd, rp->ai_addr, rp->ai_addrlen) != -1){
            break;
        }
        close(sd);
    }

    if(rp == NULL){
        cerr << "Could not connect to specified host and port" << endl;
        return EXIT_FAILURE;
    }

    freeaddrinfo(result);


    #ifdef DBG_OT

    srand((unsigned int)time(NULL));
    unsigned int i;
    bytevector m0,m1;
    int r0 = rand();
    int r1 = rand();

    //fprintf(stderr,"addr = %p, my_int[%i] = %02hhx\n",((char *)base_addr + i),i, *(char *)(base_addr+ i) );
    //fprintf(stderr,"addr = %p, my_int[%i] = %02hhx\n",((char *)base_addr + i),i, *(int *)(base_addr+ i) );
    //fprintf(stderr,"addr = %p, my_int[%i] = %02hhx\n",((char *)base_addr + i),i, *(char *)((char *)&my_int+ i) );
    //int add
    for(i = 0;i<sizeof(int);i++){
	   m0.push_back( *(char *)((char *)&r0+ i));
	   m1.push_back( *(char *)((char *)&r1+ i));
	}
    /*
    m0.push_back(0x01);
    m0.push_back(0x02);
    m0.push_back(0x03);

    m1.push_back(0x02);
    m1.push_back(0x01);
    m1.push_back(0x03);
    */
    fprintf(stderr,"[debug-ot][sender]\n");
    RSAObliviousTransfer::send(sd, m0, m1);
    exit(0);
    #endif


    //SenderEvaluator eval = SenderEvaluator( sizeof(wealth) * 8 );
    //eval.execute_protocol(sd);

    // TODO: arbitrary-precision wealth
    bytevector x = uint64_t_to_bytevector(wealth);
    printf("x = %lu = ", wealth);
    print_bytevector_as_bits(x);
    printf("\n");
    x.resize(1); //UGLY HACK TO GET 8-bit circuit

    Circuit c = generate_unsigned_compare_circuit( x.size() * 8 );
    SenderGarbledCircuit sgc(c);


    //bytevector yao_result = sgc.send<RSAObliviousTransfer>(sd, x);
    bytevector yao_result = sgc.send<TerriblyInsecureObliviousTransfer>(sd, x);
    printf("Result: ");
    print_bytevector_as_bits(yao_result);
    printf("\n");

    close(sd);
    return 0;
}

int receiver_main(int, char** argv) {
    unsigned short port = (unsigned short) atoi(argv[2]);
    uint64_t wealth = (uint64_t) atoi(argv[3]);
    (void)port; (void)wealth; // suppress -Wunused-but-set-variable
    int sd, rc;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));

    int server_sd;
    hints.ai_family = AF_UNSPEC; //IPv4 or IPv6 allowed
    hints.ai_socktype = SOCK_STREAM; //TCP
    hints.ai_flags = AI_PASSIVE; //Any IP
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    rc = getaddrinfo(NULL, argv[2], &hints, &result);
    if(rc != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return EXIT_FAILURE;
    }

    //Try to bind to each address
    for(rp = result; rp != NULL; rp = rp->ai_next){
        server_sd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(server_sd == -1){
            continue;
        }
        if(bind(server_sd, rp->ai_addr, rp->ai_addrlen) == 0){
            break;
        }
        close(server_sd);
    }

    if(rp == NULL){
        cerr << "Could not bind to specified port" << endl;
        return EXIT_FAILURE;
    }

    freeaddrinfo(result);

    rc = listen(server_sd, 1);
    struct sockaddr_in client;
    int fromlen = sizeof(client);
    sd = accept(server_sd, (struct sockaddr *) &client, (socklen_t*) &fromlen);
    if(sd < 0){
        perror("Failed to accept connection");
        return EXIT_FAILURE;
    }


    #ifdef DBG_OT
    bytevector my_msg;
    //int rand_val = rand();
    my_msg = RSAObliviousTransfer::recv(sd, 1);

    exit(0);
    #endif


    //ReceiverEvaluator eval = ReceiverEvaluator(sizeof(wealth) * 8);
    //eval.execute_protocol(sd);

    bytevector y = uint64_t_to_bytevector(wealth);
    printf("y = %lu = ", wealth);
    print_bytevector_as_bits(y);
    printf("\n");
    y.resize(1);

    //ReceiverGarbledCircuit rgc(PhantomData<RSAObliviousTransfer>(), sd, y);
    ReceiverGarbledCircuit rgc(PhantomData<TerriblyInsecureObliviousTransfer>(), sd, y);

    printf("Result: ");
    print_bytevector_as_bits(pack_bv(rgc.result));
    printf("\n");

    close(sd);
    return 0;
}

int ot_send_main(int, char** argv) {
    string hostname = argv[2];
    unsigned short port = (unsigned short) atoi(argv[3]);
    uint64_t wealth = (uint64_t) stoull(argv[4]);
    (void)hostname; (void)port; (void)wealth; // suppress -Wunused-but-set-variable
    int sd, rc;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_UNSPEC; //IPv4 or IPv6 allowed
    hints.ai_socktype = SOCK_STREAM; //TCP
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    rc = getaddrinfo(argv[2], argv[3], &hints, &result);
    if(rc != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return EXIT_FAILURE;
    }

    //Try connecting to each address
    for(rp = result; rp != NULL; rp = rp->ai_next){
        sd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(sd == -1){
            continue;
        }
        if(connect(sd, rp->ai_addr, rp->ai_addrlen) != -1){
            break;
        }
        close(sd);
    }

    if(rp == NULL){
        cerr << "Could not connect to specified host and port" << endl;
        return EXIT_FAILURE;
    }

    freeaddrinfo(result);

    srand((unsigned int)time(NULL));
    unsigned int i;
    bytevector m0,m1;
    int r0 = rand();
    int r1 = rand();

    //fprintf(stderr,"addr = %p, my_int[%i] = %02hhx\n",((char *)base_addr + i),i, *(char *)(base_addr+ i) );
    //fprintf(stderr,"addr = %p, my_int[%i] = %02hhx\n",((char *)base_addr + i),i, *(int *)(base_addr+ i) );
    //fprintf(stderr,"addr = %p, my_int[%i] = %02hhx\n",((char *)base_addr + i),i, *(char *)((char *)&my_int+ i) );
    //int add
    for(i = 0;i<sizeof(int);i++){
       m0.push_back( *(char *)((char *)&r0+ i));
       m1.push_back( *(char *)((char *)&r1+ i));
    }
    /*
    m0.push_back(0x01);
    m0.push_back(0x02);
    m0.push_back(0x03);

    m1.push_back(0x02);
    m1.push_back(0x01);
    m1.push_back(0x03);
    */
    fprintf(stderr,"[debug-ot][sender]\n");
    RSAObliviousTransfer::send(sd, m0, m1);
    close(sd);

    printf("m0: ");
    for(unsigned int i = 0; i < m0.size(); i++){
        printf("%#x ", m0[i]);
    }
    printf("\n");

    printf("m1: ");
    for(unsigned int i = 0; i < m1.size(); i++){
        printf("%#x ", m1[i]);
    }
    printf("\n");

    return 0;
}

int ot_recv_main(int, char** argv) {
    unsigned short port = (unsigned short) atoi(argv[2]);
    uint64_t wealth = (uint64_t) atoi(argv[3]);
    (void)port; (void)wealth; // suppress -Wunused-but-set-variable
    int sd, rc;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));

    int server_sd;
    hints.ai_family = AF_UNSPEC; //IPv4 or IPv6 allowed
    hints.ai_socktype = SOCK_STREAM; //TCP
    hints.ai_flags = AI_PASSIVE; //Any IP
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    rc = getaddrinfo(NULL, argv[2], &hints, &result);
    if(rc != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return EXIT_FAILURE;
    }

    //Try to bind to each address
    for(rp = result; rp != NULL; rp = rp->ai_next){
        server_sd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(server_sd == -1){
            continue;
        }
        if(bind(server_sd, rp->ai_addr, rp->ai_addrlen) == 0){
            break;
        }
        close(server_sd);
    }

    if(rp == NULL){
        cerr << "Could not bind to specified port" << endl;
        return EXIT_FAILURE;
    }

    freeaddrinfo(result);

    rc = listen(server_sd, 1);
    struct sockaddr_in client;
    int fromlen = sizeof(client);
    sd = accept(server_sd, (struct sockaddr *) &client, (socklen_t*) &fromlen);
    if(sd < 0){
        perror("Failed to accept connection");
        return EXIT_FAILURE;
    }


    bytevector my_msg;
    int rand_val = rand();
    my_msg = RSAObliviousTransfer::recv(sd, rand_val & 0x1);
    close(sd);

    printf("Bit: %d\n", rand_val & 0x1);
    printf("Received: ");
    for(unsigned int i = 0; i < my_msg.size(); i++){
        printf("%#x ", my_msg[i]);
    }
    printf("\n");
    return 0;
}

int evaluator_main(int, char** argv) {
    // TODO: arbitrary precision integers
    uint64_t wealth1_ = (uint64_t) stoull(argv[2]);
    uint64_t wealth2_ = (uint64_t) stoull(argv[3]);
    bytevector wealth1, wealth2;
    wealth1 = uint64_t_to_bytevector(wealth1_);
    wealth2 = uint64_t_to_bytevector(wealth2_);
    Circuit c = generate_unsigned_compare_circuit(8*max(wealth1.size(), wealth2.size()));
    bytevector result = eval_circuit(c, wealth1, wealth2);
    cout << result.size() << endl;
    size_t i;
    for(i=0; i<result.size(); ++i) {
        printf("result[%lu]: %hhu\n", i, result[i]);
    }
    return 0;
}

int test_main(int, char**){
    cout << "Tests byte vector packing/unpacking" << endl;
    bytevector byv;
    byv.push_back(uint8_t(0));
    byv.push_back(uint8_t(1));
    cout << bitset<8>(byv[0]) << " " << bitset<8>(byv[1]) <<endl;
    bitvector biv = unpack_bv(byv);
    unsigned int i = 0;
    for(i = 0; i < biv.size(); i++){
        cout << biv[i];
    }
    cout << endl;
    byv = pack_bv(biv);
    cout << bitset<8>(byv[0]) << " " << bitset<8>(byv[1]) << endl;
    cout << "Another number" << endl;
    byv.clear();
    byv.push_back(uint8_t(252));
    byv.push_back(uint8_t(0));
    cout << bitset<8>(byv[0]) << " " << bitset<8>(byv[1]) <<endl;
    biv = unpack_bv(byv);
    for(i = 0; i < biv.size(); i++){
        cout << biv[i];
    }
    cout << endl;
    byv = pack_bv(biv);
    cout << bitset<8>(byv[0]) << " " << bitset<8>(byv[1]) << endl;
    return 0;
}

int test_encryption_main(int, char**) {
    int fd = open("/dev/urandom", O_RDONLY);
    bytevector k1(SEC_PARAM), k2(SEC_PARAM);
    bytevector data(SEC_PARAM+1), result1(SEC_PARAM+1), result2(SEC_PARAM+1);

#define RANDOM_POPULATE(vec) read_aon(fd, (char*)(vec).data(), (vec).size());
    RANDOM_POPULATE(k1)
    RANDOM_POPULATE(k2)
    RANDOM_POPULATE(data)
#undef RANDOM_POPULATE

    printf("k1 =\t\t"); print_bytevector_as_bits(k1); printf("\n");
    printf("k2 =\t\t"); print_bytevector_as_bits(k2); printf("\n");

    encrypt(result1.data(), data.data(), k1.data(), k2.data());
    decrypt(result2.data(), result1.data(), k1.data(), k2.data());

    printf("result1 =\t"); print_bytevector_as_bits(result1); printf("\n");
    printf("data =\t\t"); print_bytevector_as_bits(data); printf("\n");
    printf("result2 =\t"); print_bytevector_as_bits(result2); printf("\n");

    close(fd);
    return 0;
}

int main(int argc, char** argv) {
    auto print_sender_usage = [&]() {
        cout << "Usage is " << argv[0] << " sender <hostname> <port> <wealth>" << endl;
    };
    auto print_receiver_usage = [&]() {
        cout << "Usage is " << argv[0] << " receiver <port> <wealth>" << endl;
    };
    auto print_evaluator_usage = [&]() {
        cout << "Usage is " << argv[0] << " evaluator <wealth1> <wealth2>" << endl;
    };
    auto print_test_usage = [&]() {
        cout << "Usage is " << argv[0] << " test" << endl;
    };

    auto print_usage = [&]() {
        print_sender_usage();
        cout << "OR" << endl;
        print_receiver_usage();
        cout << "OR" << endl;
        print_evaluator_usage();
        cout << "OR" << endl;
        print_test_usage();
    };

    if(argc < 2) {
        print_usage();
        return EXIT_FAILURE;
    }

    if(!strcmp("sender", argv[1])) {
        if(argc != 5) {
            print_sender_usage();
            return EXIT_FAILURE;
        } else {
            return sender_main(argc, argv);
        }
    }
    if(!strcmp("receiver", argv[1])) {
        if(argc != 4) {
            print_receiver_usage();
            return EXIT_FAILURE;
        } else {
            return receiver_main(argc, argv);
        }
    }
    if(!strcmp("evaluator", argv[1])) {
        if(argc != 4) {
            print_evaluator_usage();
            return EXIT_FAILURE;
        } else {
            return evaluator_main(argc, argv);
        }
    };
    if(!strcmp("test", argv[1])){
        return test_main(argc, argv);
    }
    if(!strcmp("test_encryption", argv[1])) {
        return test_encryption_main(argc, argv);
    }
    if(!strcmp("ot_send", argv[1])){
        return ot_send_main(argc, argv);
    }
    if(!strcmp("ot_recv", argv[1])){
        return ot_recv_main(argc, argv);
    }

    print_usage();
    return EXIT_FAILURE;
}
