
# This is based on https://github.com/rsmmr/install-clang/blob/master/Dockerfile
FROM        rsmmr/clang:3.5
MAINTAINER  Robin Sommer <robin@icir.org>

# Setup packages.
RUN apt-get update
RUN apt-get -y install bison flex libpapi-dev libgoogle-perftools-dev
RUN apt-get -y install libpcap-dev libssl-dev python-dev swig zlib1g-dev
RUN apt-get -y install gdb

# Setup environment.
ENV PATH $PATH:/usr/local/bro/bin:/opt/bro/aux/btest
ENV PATH $PATH:/opt/hilti/tools:/opt/hilti/build/tools::/opt/hilti/build/tools/pac-driver

# Put a small example in place.
WORKDIR /root
RUN ( echo 'module Test;'; echo; echo 'print "Hello, world!";' ) >hello-world.pac2

# Default to run upon container startup.
CMD hilti-config --version; bash

# Setup Bro
RUN cd /opt && git clone --recursive git://git.bro.org/bro
RUN cd /opt/bro && ./configure && make -j 5 && make install

# Setup HILTI.
ADD . /opt/hilti
RUN cd /opt/hilti && make BRO_DIST=/opt/bro



