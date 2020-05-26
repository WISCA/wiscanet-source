all: comp install
comp:
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
