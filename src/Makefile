all:
	cd cnode/src; make
	cd enode; make

clean:
	cd cnode/src; make clean
	cd enode; make clean
	cd build/cnode/bin; rm -f *
	cd build/cnode/log; rm -f *
	cd build/enode/bin; rm -f *
	cd build/enode/mat/lib; rm -f *
	cd build/enode/tools; rm -f *

install:
	cd cnode/src; make install
	cd enode; make install
