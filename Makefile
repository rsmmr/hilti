#
# This is just a wrapper around the CMake setup.
#

GIT=git://git.icir.org/hilti

DOCKER_IMAGE=rsmmr/hilti
DOCKER_TMP=/xa/robin/hilti-docker-build

all: release

debug:
	test -d build || mkdir build
	( cd build; test -e Makefile || cmake  -D CMAKE_BUILD_TYPE=Debug -D BRO_DIST=$${BRO_DIST} -D LLVM_CONFIG_EXEC=$${LLVM_CONFIG_EXEC} ..; $(MAKE) )

release:
	test -d build || mkdir build
	( cd build; test -e Makefile || cmake  -D CMAKE_BUILD_TYPE=RelWithDebInfo -D BRO_DIST=$${BRO_DIST} -D LLVM_CONFIG_EXEC=$${LLVM_CONFIG_EXEC} ..; $(MAKE) )

clean:
	rm -rf build
	( cd doc && $(MAKE) clean )

test:
	-( cd tests && btest -j -f diag.log )
	-( cd bro/tests && btest -j -f diag.log )

tags:
	update-tags

docker-build:
	rm -rf $(DOCKER_TMP)
	mkdir -p $(DOCKER_TMP)
	(cd $(DOCKER_TMP) && git clone ${GIT})
	cp Makefile $(DOCKER_TMP)/hilti
	cp Dockerfile $(DOCKER_TMP)/hilti
	(cd $(DOCKER_TMP)/hilti && make docker-build-internal)

docker-build-internal:
	docker build -t ${DOCKER_IMAGE} .
	docker tag -f `docker inspect --format='{{.Id}}' ${DOCKER_IMAGE}` ${DOCKER_IMAGE}:`cat VERSION`
	docker tag -f `docker inspect --format='{{.Id}}' ${DOCKER_IMAGE}` ${DOCKER_IMAGE}:latest

docker-run:
	docker run -i -t ${DOCKER_IMAGE}

docker-push:
	docker login
	docker push ${DOCKER_IMAGE}
