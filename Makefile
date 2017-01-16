#
# This is just a wrapper around the CMake setup.
#

BUILD=build

DOCKER_TAG=rsmmr/hilti
DOCKER_VERSION=`cat VERSION`
DOCKER_IMAGE=rsmmr/hilti
DOCKER_TMP=hilti-docker-build.tmp

all: configured
	$(MAKE) -C $(BUILD) $@

clean:
	rm -rf build $(DOCKER_TMP)
	( cd doc && $(MAKE) clean )

test:
	-( cd tests && btest -j -f diag.log )
	-( cd bro/tests && btest -j -f diag.log )

configured:
	@test -d $(BUILD) || ( echo "Error: No build/ directory found. Did you run configure?" && exit 1 )
	@test -e $(BUILD)/Makefile || ( echo "Error: No build/Makefile found. Did you run configure?" && exit 1 )

clang-format:
	@test $$(clang-format -version | cut -d ' ' -f 3 | sed 's/\.//g') -ge 390 || (echo "Must have clang-format >= 3.9"; exit 1)
	@(clang-format -dump-config | grep -q SpacesAroundConditions) || (echo "Must have patched version of clang-format"; exit 1)
	clang-format -i $$(scripts/source-files)

### Docker targets.

docker-check:
	@test $$(docker info 2>/dev/null | grep "Base Device Size" | awk '{print int($$4)}') -ge 30 \
     || (echo "Increase Docker base device size to 30g, see http://www.projectatomic.io/blog/2016/03/daemon_option_basedevicesize/" \
     && false)

docker-build:
	rm -rf $(DOCKER_TMP)
	mkdir -p $(DOCKER_TMP)
	(export hilti=$$(pwd); cd $(DOCKER_TMP) && git clone $$hilti hilti)
	cp Makefile $(DOCKER_TMP)/hilti
	cp Dockerfile $(DOCKER_TMP)/hilti
	(cd $(DOCKER_TMP)/hilti && make docker-build-internal)

docker-build-internal:
	docker build -t $(DOCKER_TAG):$(DOCKER_VERSION) -f Dockerfile .
	docker tag $$(docker inspect --format='{{.Id}}' $(DOCKER_TAG):$(DOCKER_VERSION)) $(DOCKER_TAG):latest

docker-run:
	docker run -i -t ${DOCKER_TAG}:latest

docker-push:
	docker login
	docker push ${DOCKER_TAG}:latest
