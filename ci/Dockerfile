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

FROM nexus3.o-ran-sc.org:10004/bldr-ubuntu18-c-go:2-u18.04-nng

# Install build tools and SDL dependencies:
RUN apt-get update && \
    apt-get install -y libhiredis-dev && \
    apt-get clean

# Copy sourcefiles:
RUN mkdir -p /tmp/build
COPY . /tmp/build

# Compile and run unit tests:
WORKDIR /tmp/build
RUN ./autogen.sh && \
    ./configure && \
    make all && \
    make test
