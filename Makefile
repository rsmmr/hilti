#
# This is just a wrapper around the CMake setup.
#

DOCKER_IMAGE="rsmmr/hilti"

all: release

debug:
	test -d build || mkdir build
	( cd build; test -e Makefile || cmake  -D CMAKE_BUILD_TYPE=Debug -D BRO_DIST=$${BRO_DIST} ..; $(MAKE) )

release:
	test -d build || mkdir build
	( cd build; test -e Makefile || cmake  -D CMAKE_BUILD_TYPE=RelWithDebInfo -D BRO_DIST=$${BRO_DIST} ..; $(MAKE) )

clean:
	rm -rf build
	( cd doc && $(MAKE) clean )

tags:
	update-tags

docker-build:
	docker build -t ${DOCKER_IMAGE} .

docker-run:
	docker run -i -t ${DOCKER_IMAGE} /bin/bash
