
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
ENV BRO_PLUGIN_PATH /opt/hilti/build/bro

# Put a couple small examples in place.
WORKDIR /root
ADD docker/ .

# Default to run upon container startup.
CMD hilti-config --version; bash

# Setup Bro
RUN cd /opt && git clone -b release/2.4 --recursive git://git.bro.org/bro
RUN cd /opt/bro && CXX="/opt/llvm/bin/clang++ --stdlib=libc++" ./configure && make -j 5 && make install

# Setup HILTI.
ADD . /opt/hilti
RUN cd /opt/hilti && make -j 5 BRO_DIST=/opt/bro



