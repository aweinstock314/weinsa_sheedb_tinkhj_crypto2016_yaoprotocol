.PHONY: all clean

MITIGATIONS=-fstack-protector-all -D_FORTIFY_SOURCE=2 -fPIC -Wl,-zrelro,-znow

CCFLAGS=-O2 ${MITIGATIONS} -std=c++11 -Wall -Wextra -pedantic -Werror

all: bin/yaoprotocol

bin/yaoprotocol: src/yaoprotocol.cpp
	mkdir -p bin
	g++ ${CCFLAGS} $< -o $@ -lcryptopp

clean:
	rm -r ./bin
