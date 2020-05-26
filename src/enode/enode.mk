ENODE_PARSER = ../shared/cnfParse.cpp

OUT=enode
OPT=-std=c++17 -Wall -DSUDO_MODE
INC=-I../include/
LIB=-lpthread -ltinyxml

all:
	g++ $(OPT) -o $(OUT) $(LIB) $(INC) enode.cpp $(ENODE_PARSER)

clean:
	rm -rf ./$(OUT) ./a.out

install:
	mkdir -p ../build/enode/bin
	cp $(OUT) ../build/enode/bin
