MEX_OCT := $(shell mex -h 2>/dev/null)

all: mex install

mex:
ifdef MEX_OCT
	@echo "Found MATLAB; Using MEX to compile mex targets"
	mex gradio_mex.c
	mex local_usrp_mex.c
else
	@echo "MATLAB not found; Using Octave to compile mex targets"
	mkoctfile --mex gradio_mex.c
	mkoctfile --mex local_usrp_mex.c
	cp gradio_mex.mex gradio_mex.mexa64
	cp local_usrp_mex.mex local_usrp_mex.mexa64
endif

install:
	mkdir -p ../../build/enode/mat/lib/
	cp *.m      ../../build/enode/mat/lib
ifdef MEX_OCT
	cp *.mexa64 ../../build/enode/mat/lib
else
	cp *.mex ../../build/enode/mat/lib
endif

clean:
	rm *.mex*
