#
#   Copyright (c) 2018-2019 Nokia.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

#
#   This source code is part of the near-RT RIC (RAN Intelligent Controller)
#   platform project (RICP).
#

FROM ubuntu:18.04

# Install building tools and SDL dependencies:
RUN apt-get update && \
    apt-get install -y \
        build-essential \
        automake \
        libtool \
        pkg-config \
        libgtest-dev \
        rpm \
        devscripts \
        gawk \
        debhelper \
        autoconf-archive \
        libboost-filesystem-dev \
        libboost-program-options-dev \
        libboost-system-dev \
        libhiredis-dev \
        valgrind && \
    apt-get clean

# Copy sourcefiles:
RUN mkdir -p /tmp/build
COPY . /tmp/build
WORKDIR /tmp/build

# Compile and run unit tests:
RUN ./autogen.sh && \
    ./configure && \
    make all && \
    make test

# Build packages:
RUN make clean && \
    make distclean && \
    ./configure --with-rpm-dir=/tmp/pkgs && \
    make rpm-pkg && \
    make clean && \
    make distclean && \
    ./configure --with-deb-dir=/tmp/pkgs && \
    make deb-pkg

RUN cp /tmp/build/docker_test/publish.sh /bin
ENTRYPOINT ["/bin/publish.sh"]
