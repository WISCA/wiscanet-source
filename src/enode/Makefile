dateStr="char dateStr[128] = \"$(shell date)\";"

all: comp install
comp:
	cp ../verStr.h .
	echo $(dateStr) >> ./verStr.h
	make -f enode.mk
	make -f uControl.mk
	cd mat && make

clean:
	make -f enode.mk clean
	make -f uControl.mk clean
	cd mat && make clean

install:
	make -f enode.mk install
	make -f uControl.mk install
	cd mat && make install
