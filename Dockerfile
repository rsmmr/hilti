FROM        ubuntu:xenial
MAINTAINER  Robin Sommer <robin@icir.org>

RUN echo "deb http://apt.llvm.org/xenial     llvm-toolchain-xenial-3.9 main" >>/etc/apt/sources.list
RUN echo "deb-src http://apt.llvm.org/xenial llvm-toolchain-xenial-3.9 main" >>/etc/apt/sources.list

RUN apt-get -y update
RUN apt-get -y install cmake git build-essential vim python curl ninja-build
RUN apt-get -y install bison flex libpapi-dev libgoogle-perftools-dev
RUN apt-get -y install libpcap-dev libssl-dev python-dev swig zlib1g-dev
RUN apt-get -y install gdb
RUN apt-get -y --allow-unauthenticated install clang-3.9 llvm-3.9 lldb-3.9

RUN cd /usr/bin && for i in $(ls *-3.9); do ln -s $i $(echo $i | sed 's/-3.9//g'); done

ENV PATH $PATH:/usr/local/bro/bin:/opt/bro/aux/btest
ENV PATH $PATH:/opt/hilti/tools:/opt/hilti/build/tools:/opt/hilti/build/tools/spicy-driver:/opt/hilti/build/tools/spicy-dump
ENV BRO_PLUGIN_PATH /opt/hilti/build/bro

# Default to run upon container startup.
CMD hilti-config --version; spicy-config --version; bash

# Put a couple small examples in place.
WORKDIR /root
ADD docker/ .

# Set up Bro
RUN mkdir -p /opt/bro/src
RUN cd /opt/bro && git clone -b release/2.5 --recursive git://git.bro.org/bro src
RUN cd /opt/bro/src && ./configure --generator=Ninja && cd build && ninja && ninja install && cd ..

# Set up HILTI.
ADD . /opt/hilti
RUN cd /opt/hilti && ./configure --bro-dist=/opt/bro/src --generator=Ninja && cd build && ninja
