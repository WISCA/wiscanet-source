LIB  = -lpthread -lboost_program_options -luhd -lboost_thread -lboost_filesystem
SRC  = uControl.cpp 

all:
	g++ -o uControl $(LIB) $(INC) $(SRC)

clean:
	rm -f uControl

install:
	cp uControl ../../build/enode/bin
