ENODE_PARSER = ../shared/cnfParse.cpp

OUT=cnode
OPT=-std=c++17 -Wall
INC=-I../include/
LIB=-lpthread -lssl -lcrypto -ltinyxml

all: cnode install

cnode:
	g++ $(OPT) -o $(OUT) $(LIB) $(INC) cnode.cpp $(ENODE_PARSER)

clean:
	rm -rf ./$(OUT) ./a.out

install:
	mkdir -p ../build/cnode/bin/
	mkdir -p ../build/cnode/log/
	cp $(OUT) ../build/cnode/bin/
