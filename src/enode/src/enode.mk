#TXML = ../../lib/tinyxml
#TXML_INC = $(TXML)
#TXML_SRC = $(TXML)/tinyxml.cpp $(TXML)/tinystr.cpp \
           $(TXML)/tinyxmlerror.cpp $(TXML)/tinyxmlparser.cpp \

ENODE_PARSER = cnfParse.cpp

OUT=enode
OPT=-std=c++0x -DSUDO_MODE
INC=-I../../hdr/
LIB=-lpthread -ltinyxml

all:
	g++ $(OPT) -o $(OUT) $(LIB) $(INC) enode.cpp $(ENODE_PARSER)

clean:
	rm -rf ./$(OUT) ./a.out

install:
	cp $(OUT) ../../build/enode/bin
