..
..  Copyright (c) 2019 AT&T Intellectual Property.
..  Copyright (c) 2019 Nokia.
..
..  Licensed under the Creative Commons Attribution 4.0 International
..  Public License (the "License"); you may not use this file except
..  in compliance with the License. You may obtain a copy of the License at
..
..    https://creativecommons.org/licenses/by/4.0/
..
..  Unless required by applicable law or agreed to in writing, documentation
..  distributed under the License is distributed on an "AS IS" BASIS,
..  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
..
..  See the License for the specific language governing permissions and
..  limitations under the License.
..


###############
Developer Guide
###############

.. raw:: pdf

   PageBreak

.. contents::
   :depth: 3
   :local:

.. raw:: pdf

   PageBreak

Introduction
************

This is the developer guide of O-RAN SC SDL C++ library.
SDL implementation downloading (execute command in such directory where you want
to download SDL repository).

Without commit hook::

    git clone "https://gerrit.o-ran-sc.org/r/ric-plt/sdl"

With commit hook::

    git clone "https://gerrit.o-ran-sc.org/r/ric-plt/sdl" && (cd "sdl" && mkdir -p .git/hooks && curl -Lo `git rev-parse --git-dir`/hooks/commit-msg https://gerrit.o-ran-sc.org/r/tools/hooks/commit-msg; chmod +x `git rev-parse --git-dir`/hooks/commit-msg)


SDL has only few dependencies to other components and therefore SDL should be
quite simple to build and install to almost any modern Linux environment by
following instructions below.

If not otherwise mentioned, commands in this document are executed in the
directory where the SDL repository has been cloned into.

.. raw:: pdf

   PageBreak

Compilation
***********

**Dependencies**

C++ compiler supporting C++14 is required for compiling SDL.

Currently, the following library is required while building:

* boost
* hiredis
* doxygen (optional)

Commands to install dependent packages in Fedora::

    sudo dnf install boost-devel
    sudo dnf install hiredis-devel
    sudo dnf install doxygen

Commands to install dependent packages in Debian/Ubuntu::

    sudo apt install libboost-all-dev
    sudo apt install libhiredis-dev
    sudo apt install doxygen

**Compilation in the Source Directory**::

    ./autogen.sh
    ./configure
    make all
    make test

**Compilation in a Separate Build Directory**

Both *configure* and *make* can be executed in a separate directory
(or directories) for keeping the source directory clean or for testing
different configuration options in parallel. For example::

    ./autogen.sh
    mkdir build
    cd build
    ../configure
    make all
    make test

Note that if you compile SDL this way, all coming *configure* and *make*
commands are executed in the build directory like above.

.. raw:: pdf

   PageBreak

Installation
************

By default the shared library is installed to */usr/local/lib* and headers into
to */usr/local/include*. If this is not desired, then provide different path
when running *configure*, for example::

    ./configure --prefix=$HOME

Note that *configure* command allows plethora of additional options. For more
info::

    ./configure --help

After configuration has been done, run::

    make install

.. raw:: pdf

   PageBreak

.. _building_sdl_api_doc:

Building SDL API Document
*************************

SDL API Documentation is a Doxygen document generated from SDL public header
files.

One can generate Doxygen documentation locally by running commands::

    ./autogen.sh
    ./configure
    make doxygen-doc

in the directory where the SDL repository has been cloned to.


By default make doxygen-doc creates HTML, PDF and PS documents (if the needed
tools are available, check the output of *./configure* command to get the names
of missing tools). The documents are created to (paths are relative to the
directory where the SDL repository has been cloned to):

* doxygen-doc/html/index.html
* doxygen-doc/shareddatalayer.pdf
* doxygen-doc/shareddatalayer.ps


Creation of different document formats can be controlled with various
--enable-doxygen-* and --disable-doxygen-* configuration options. For example
if only HTML document is needed, then run::

    ./configure --disable-doxygen-pdf --disable-doxygen-ps
    make doxygen-doc

.. raw:: pdf

   PageBreak

Running Tests
*************

Unit Tests
==========

Unit tests are compiled and executed by simply running::

    make test

By default all unit tests are executed. If *valgrind* is installed, then by
default unit test execution is analyzed with *valgrind*.

Running specific test cases can be achieved by using *GTEST_FILTER* environment
variable. For example::

    make test GTEST_FILTER=AsyncStorageTest*

If *valgrind* is not desired (even if installed), then it can be disabled with
*USE_VALGRIND* environment variable::

    make test USE_VALGRIND=false

Additional *valgrind* arguments can be specified with *VALGRIND_EXTRA_ARGS*
environment variable. For example::

    make test VALGRIND_EXTRA_ARGS='--track-fds=yes --log-file=mylog.txt'

It is also possible to use the *testrunner* binary directly (after it has been
compiled). The *testrunner* binary supports all the command line options gtest
supports, for example::

    make testrunner
    ./testrunner --help
    ./testrunner
    ./testrunner --gtest_filter=AsyncStorageTest*

To get unit test code coverage analysis enable unit test gcov code coverage
analysis by configuring gcov reporting directory::

    configure --with-gcov-report-dir=DIR

Directory can be an absolute path or a relative path to an SDL source root.
Unit test build creates directory if it does not exist.

Build and run unit tests with code coverage analysis::

    make test_gcov

After successful unit test run code coverage (.gcov) result files are in
a directory, what was defined by '--with-gcov-report-dir' configure option.

In addition, graphical gcov front-ends such as lcov can be used for coverage
analysis::

    lcov --directory tst/ --directory src --capture --output-file coverage.info
    genhtml coverage.info --output-directory out

Open the out/index.html using any web browser.


Docker Tests
============

It's also possible to test SDL compilation, run unit tests and test building of
rpm and Debian packages in a Docker::

    docker build  --no-cache -f docker_test/Dockerfile-Test -t sdltest:latest .

If needed, ready rpm and Debian packages can be copied from Docker to host. In
below example packages are copied to host's /tmp/sdltest-packages directory::

    docker run -v /tmp/sdltest-packages:/export sdltest:latest /export

Functional Tests
================

Functional tests are not yet available.

.. raw:: pdf

   PageBreak

Debugging SDL Binaries
**********************

The testrunner and other binaries created by make into the working
directory are not real binaries but shell scripts (generated by automake)
used for running the real binaries. The real binaries are in .libs directory.
Normally this is not important, but when using gdb or other debugger/analyzer
tools it is important to use the real binary instead of the generated shell
script.

Examples below demonstrate how one can run testrunner binary (unit tests) in
gdb debugger::

    LD_LIBRARY_PATH=.libs gdb --args .libs/testrunner
    LD_LIBRARY_PATH=.libs gdb --args .libs/testrunner --gtest_filter=AsyncStorageTest*

.. raw:: pdf

   PageBreak

Redis
*****

When Redis type backend data storage is used, SDL requires Redis v4.0 or
greater. Older versions do not support extension modules.

Redis Modules
=============

When Redis type backend data storage is used, SDL requires that the following
Redis extension commands have been installed to runtime environment:

* MSETPUB
* SETIE
* SETIEPUB
* SETNXPUB
* DELPUB
* DELIE
* DELIEPUB

Implementation for these commands is produced by RIC DBaaS. In official RIC
deployments these commands are installed by DBaaS service to Redis
container(s). In development environment you may want install commands
manually to pod/container which is running Redis.

**Manual Redis module installing**

Redis module implementation downloading (execute command in such directory
where you want to download redis module implementation)::

    git clone "https://gerrit.o-ran-sc.org/r/ric-plt/dbaas"

Installation to default system directory::

    cd redismodule
    ./autogen.sh
    ./configure
    make install

Following line should be added to *redis.conf* file::

    loadmodule <path>/libredismodule.so

<path> should be replaced to match library installation directory path.

*redis-server* must be restarted after configuration file update.

Notice that *redis-server* takes path to configuration file as an argument.
If not given, *redis-server* will start with default parameter values and above
made *loadmodule* option is not effective. Refer to::

    redis-server --help

SDL API will check in connection setup phase that all required Redis extension
commands are available, if not then execution is aborted and error log is
written to identify which commands are missing.

.. raw:: pdf

   PageBreak
