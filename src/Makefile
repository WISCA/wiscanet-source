dateStr="char dateStr[128] = \"$(shell date)\";"
verStr="char verStr[128] = \"V.0.7.1\";"

all:
	echo $(verStr) > include/verStr.h
	echo $(dateStr) >> include/verStr.h
	cd cnode; make
	cd enode; make

clean:
	cd cnode; make clean
	cd enode; make clean
	rm -rf build

install:
	cd cnode; make install
	cd enode; make install
