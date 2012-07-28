# $Id$
#
# Build compiler and libhilti. This Makefile builds both debug and
# release versions of libhilti simultaniously.

all: all-debug

all-debug:
	test -d build || mkdir build
	( cd build; test -e Makefile || cmake  -D CMAKE_BUILD_TYPE=Debug ..; $(MAKE) )

####

debug: mkdirs libhilti-debug

release: mkdirs libhilti-releae

hilti-debug:
	( cd build; test -e Makefile || cmake  -D CMAKE_BUILD_TYPE=Debug ..; $(MAKE) )

hilti-release:
	( cd build; test -e Makefile || cmake  -D CMAKE_BUILD_TYPE=RelWithDebInfo ..; $(MAKE) )

libhilti-release: hilti-release
	( cd build/libhilti-release; test -e Makefile || cmake -D CMAKE_BUILD_TYPE=RelWithDebInfo ../../libhilti; $(MAKE) )

libhilti-debug: hilti-debug
	( cd build/libhilti-debug;   test -e Makefile || cmake -D CMAKE_BUILD_TYPE=Debug          ../../libhilti; $(MAKE) )

mkdirs:
	test -d build || mkdir build
	test -d build/libhilti-debug || mkdir build/libhilti-debug
	test -d build/libhilti-release || mkdir build/libhilti-release

clean:
	rm -rf build
	( cd doc && $(MAKE) clean )

tags:
	update-tags
