# $Id$
#
# Build libhilti

-include ${HILTIDEPS}/bin/setup-paths.mk

all: do-build-llvm do-build-native

do-build-llvm:
	test -e build-llvm || mkdir build-llvm
	cd build-llvm; test -e Makefile || cmake -DENABLE_LLVM=1 ..; make VERBOSE=1

do-build-native:
	test -e build-native || mkdir build-native
	cd build-native; test -e Makefile || cmake -DENABLE_LLVM=0 ..; make VERBOSE=1

clean: 
	rm -rf build-llvm build-native

doc:
	cd build-native; make doc-libhilti

