# $Id$
#
# Build libhilti

all: do-build-llvm do-build-native

do-build-llvm:
	test -e build-llvm || mkdir build-llvm
	cd build-llvm; test -e Makefile || cmake -DENABLE_LLVM=1 ..; make 

do-build-native:
	test -e build-native || mkdir build-native
	cd build-native; test -e Makefile || cmake -DENABLE_LLVM=0 ..; make 

clean: 
	rm -rf build-llvm build-native
    
