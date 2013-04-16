#
# This is just a wrapper around the CMake setup.
#

all: release

debug:
	test -d build || mkdir build
	( cd build; test -e Makefile || cmake  -D CMAKE_BUILD_TYPE=Debug ..; $(MAKE) )

release:
	test -d build || mkdir build
	( cd build; test -e Makefile || cmake  -D CMAKE_BUILD_TYPE=RelWithDebInfo ..; $(MAKE) )

clean:
	rm -rf build
	( cd doc && $(MAKE) clean )

tags:
	update-tags
