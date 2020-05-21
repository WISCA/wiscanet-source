all: mex install

mex:
	mex gradio_mex.c
	mex local_usrp_mex.c

install:
	mkdir -p ../../build/enode/mat/lib/
	cp *.m      ../../build/enode/mat/lib
	cp *.mexa64 ../../build/enode/mat/lib
