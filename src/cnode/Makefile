#TXML = ../../lib/tinyxml
#TXML_INC = $(TXML)
#TXML_SRC = $(TXML)/tinyxml.cpp $(TXML)/tinystr.cpp \
           $(TXML)/tinyxmlerror.cpp $(TXML)/tinyxmlparser.cpp \

ENODE_PARSER = ../enode/src/cnfParse.cpp

OUT=cnode
OPT=-std=c++17 -Wall
INC=-I../hdr/ -I../enode/src/
#-I$(TXML_INC)
LIB=-lpthread -lssl -lcrypto -ltinyxml

dateStr="char dateStr[128] = \"$(shell date)\";"

all: cnode install

cnode:
	cp ../../verStr.h .
	echo $(dateStr) >> ./verStr.h
	g++ $(OPT) -o $(OUT) $(LIB) $(INC) main.cpp $(ENODE_PARSER)

clean:
	rm -rf ./$(OUT) ./a.out

install:
	mkdir -p ../build/cnode/bin/
	mkdir -p ../build/cnode/log/
	cp $(OUT) ../build/cnode/bin/
