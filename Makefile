# $Id$
#
# Build compiler and libhilti. This Makefile builds both debug and
# release versions of libhilti simultaniously.

all: debug

debug: mkdirs libhilti-debug hilti-prof

release: mkdirs libhilti-releae hilti-prof

hilti-debug:
	( cd build; test -e Makefile || cmake  -D CMAKE_BUILD_TYPE=Debug ..; $(MAKE) )

hilti-release:
	( cd build; test -e Makefile || cmake  -D CMAKE_BUILD_TYPE=RelWithDebInfo ..; $(MAKE) )

libhilti-release: hilti-release
	( cd build/libhilti-release; test -e Makefile || cmake -D CMAKE_BUILD_TYPE=RelWithDebInfo ../../libhilti; $(MAKE) )

libhilti-debug: hilti-debug
	( cd build/libhilti-debug;   test -e Makefile || cmake -D CMAKE_BUILD_TYPE=Debug          ../../libhilti; $(MAKE) )

hilti-prof: libhilti-release libhilti-debug
	( cd build; make hilti-prof )

mkdirs:
	test -d build || mkdir build
	test -d build/libhilti-debug || mkdir build/libhilti-debug
	test -d build/libhilti-release || mkdir build/libhilti-release

clean:
	rm -rf build
	( cd doc && $(MAKE) clean )

tags:
	update-tags
