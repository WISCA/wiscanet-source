TXML = ../../lib/tinyxml
TXML_INC = $(TXML)
TXML_SRC = $(TXML)/tinyxml.cpp $(TXML)/tinystr.cpp \
           $(TXML)/tinyxmlerror.cpp $(TXML)/tinyxmlparser.cpp \
           ../../enode/src/cnfParse.cpp

OUT=cnode
OPT=-std=c++0x
INC=-I../../hdr/ -I../../enode/src/ -I$(TXML_INC)
LIB=-lpthread -lssl -lcrypto

dateStr="char dateStr[128] = \"$(shell date)\";"

all:
	cp ../../../verStr.h . 
	echo $(dateStr) >> ./verStr.h
	g++ $(OPT) -o $(OUT) $(LIB) $(INC) main.cpp $(TXML_SRC)
	cp $(OUT) ../../build/cnode/bin

clean:
	rm -rf ./$(OUT) ./a.out

install:
	cp $(OUT) ../../build/cnode/bin
