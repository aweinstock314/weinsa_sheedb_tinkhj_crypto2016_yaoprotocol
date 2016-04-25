.PHONY: all clean

MITIGATIONS=-fstack-protector-all -D_FORTIFY_SOURCE=2 -fPIC -Wl,-zrelro,-znow

CCFLAGS=-O2 ${MITIGATIONS} -std=c++1y -Wall -Wextra -pedantic -Werror

all: bin/yaoprotocol

bin/yaoprotocol: src/yaoprotocol.cpp src/utils.cpp src/Circuit.cpp src/ReceiverEvaluator.cpp src/SenderEvaluator.cpp src/GarbledCircuit.cpp src/oblivious_transfer.cpp #src/utils.h src/Circuit.h src/ReceiverEvaluator.h src/SenderEvaluator.h src/oblivious_transfer.h
	mkdir -p bin
	g++ ${CCFLAGS} $^ -o $@ -lcryptopp -lpthread

ot_dbg: bin/yaoprotocol_
bin/yaoprotocol_: src/yaoprotocol.cpp src/utils.cpp src/Circuit.cpp src/ReceiverEvaluator.cpp src/SenderEvaluator.cpp src/GarbledCircuit.cpp src/oblivious_transfer.cpp #src/utils.h src/Circuit.h src/ReceiverEvaluator.h src/SenderEvaluator.h src/oblivious_transfer.h

	mkdir -p bin
	g++ ${CCFLAGS} $^ -o $@ -lcryptopp -DDBG_OT


clean:
	rm -r ./bin
