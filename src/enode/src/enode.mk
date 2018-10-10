TXML = ../../lib/tinyxml
TXML_INC = $(TXML)
TXML_SRC = $(TXML)/tinyxml.cpp $(TXML)/tinystr.cpp \
           $(TXML)/tinyxmlerror.cpp $(TXML)/tinyxmlparser.cpp \
           cnfParse.cpp

OUT=enode
OPT=-std=c++0x -DSUDO_MODE
INC=-I../../hdr/ -I$(TXML_INC)
LIB=-lpthread

all:
	g++ $(OPT) -o $(OUT) $(LIB) $(INC) enode.cpp $(TXML_SRC)

clean:
	rm -rf ./$(OUT) ./a.out

install:
	cp $(OUT) ../../build/enode/bin
