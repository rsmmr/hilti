# $Id$
#
# Build compiler and libhilti. This Makefile builds both debug and
# release versions of libhilti simultaniously.

all: debug

debug: mkdirs libhilti
	( cd build; test -e Makefile || cmake  -D CMAKE_BUILD_TYPE=Debug ..; $(MAKE) )

release: mkdirs libhilti
	( cd build; test -e Makefile || cmake  -D CMAKE_BUILD_TYPE=RelWithDebInfo ..; $(MAKE) )

mkdirs:
	test -d build || mkdir build
	test -d build/libhilti-debug || mkdir build/libhilti-debug
	test -d build/libhilti-release || mkdir build/libhilti-release

libhilti: mkdirs
	( cd build/libhilti-release; test -e Makefile || cmake -D CMAKE_BUILD_TYPE=RelWithDebInfo ../../libhilti; $(MAKE) )
	( cd build/libhilti-debug;   test -e Makefile || cmake -D CMAKE_BUILD_TYPE=Debug          ../../libhilti; $(MAKE) )

clean:
	rm -rf build
	( cd doc && $(MAKE) clean )

tags:
	update-tags
