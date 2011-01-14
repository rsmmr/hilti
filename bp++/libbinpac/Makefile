# $Id$
#
# Build libhilti

-include ${HILTIDEPS}/bin/setup-paths.mk

all: do-build-release do-build-debug

do-build-release:
	test -e build-release || mkdir build-release
	cd build-release; test -e Makefile || cmake -DLLVM_ENABLE_DEBUG=0 ..; make

do-build-debug:
	test -e build-debug || mkdir build-debug
	cd build-debug; test -e Makefile || cmake -DLLVM_ENABLE_DEBUG=1 ..; make

clean:
	rm -rf build-release build-debug
