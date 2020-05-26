dateStr="char dateStr[128] = \"$(shell date)\";"

all:
	echo "char verStr[128] = "V.0.7.1"" > include/verStr.h
	echo $(dateStr) >> include/verStr.h
	cd cnode; make
	cd enode; make

clean:
	cd cnode; make clean
	cd enode; make clean
	cd build/cnode/bin && rm -f *
	cd build/cnode/log && rm -f *
	cd build/enode/bin && rm -f *
	cd build/enode/mat/lib && rm -f *
	cd build/enode/tools && rm -f *

install:
	cd cnode; make install
	cd enode; make install
