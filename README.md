# shareddatalayer

## Table of contents

- [Documentation](#documentation)
- [Compilation](#compilation)
- [Installation](#installation)
- [Running unit tests](#running-unit-tests)
- [Using SDL in application pod](#using-sdl-in-application-pod)

## Documentation

Documentation is generated with `doxygen` tool. Dependency to `doxygen`
tool is optional. If not installed, then `doxygen-doc` target will not
be created to Makefile.

By default `make doxygen-doc` creates HTML, PDF and PS documents (if
the needed tools are available). The documents are created to:

1. doxygen-doc/html/index.html
2. doxygen-doc/shareddatalayer.pdf
3. doxygen-doc/shareddatalayer.ps

Creation of different document formats can be controlled with various
`--enable-doxygen-*` and `--disable-doxygen-*` configuration
options. For example if only HTML document is needed, then run:

    ./configure --disable-doxygen-pdf --disable-doxygen-ps
    make doxygen-doc

## Compilation

These instructions assume that the project has been cloned into
directory named `shareddatalayer`.

### Dependencies

Build-time dependencies:

    libboost (system, filesystem, program-options)
    hiredis
    rpm
    valgrind
    autoconf-archive
    doxygen (optional)

Commands to install dependent packages in Fedora:

    sudo dnf install boost-devel
    sudo dnf install hiredis-devel
    sudo dnf install rpm
    sudo dnf install valgrind
    sudo dnf install autoconf-archive
    sudo dnf install doxygen

Commands to install dependent packages in Debian/Ubuntu:

    sudo apt install libboost-filesystem-dev
    sudo apt install libboost-program-options-dev
    sudo apt install libboost-system-dev
    sudo apt install libhiredis-dev
    sudo apt install rpm
    sudo apt install valgrind
    sudo apt install autoconf-archive
    sudo apt install doxygen

### Compilation in the source directory

    cd shareddatalayer
    ./autogen.sh
    ./configure
    make all test

### Compilation outside the source directory

    cd shareddatalayer
    ./autogen.sh
    cd ..
    mkdir workdir
    cd workdir
    ../shareddatalayer/configure
    make all test

## Installation

By default the shared library is installed to `/usr/local/lib` and
headers into to `/usr/local/include`. If this is not desired, then
provide different path when running `configure`, for example:

    ./configure --prefix=$HOME/usr/local

In above example SDL pkg-config .pc file is installed to `$HOME/usr/local/lib/pkgconfig`
directory. This directory should be included to `PKG_CONFIG_PATH` while building
the application which is using the SDL API library.

Note that `configure` command allows plethora of additional options.
For more info:

    ./configure --help

After configuration has been done, run:

    make install

In some cases dynamic linker cache needs to be manually refreshed after SDL API
has been re-installed:

    ldconfig

### Redis modules

When Redis is used, SDL requires that the following Redis extension commands
have been installed to runtime environment:
- MSETPUB
- SETIE
- SETIEPUB
- SETNXPUB
- DELPUB
- DELIE
- DELIEPUB

Redis v4.0 or greater is required. Older versions do not support extension
modules.

Implementation for these commands is produced by RIC DBaaS:
https://gerrit.o-ran-sc.org/r/admin/repos/ric-plt/dbaas

In official RIC deployments these commands are installed by `dbaas` service to
Redis container(s).

In development environment you may want install commands manually to pod/container
which is running Redis.

Installation to default system directory:

    cd redismodule
    ./autogen.sh
    ./configure
    make install

Following line should be added to `redis.conf` file:

    loadmodule <path>/libredismodule.so

`<path>` should be replaced to match library installation directory path.
`redis-server` must be restarted after configuration file update.

Notice that `redis-server` takes path to configuration file as an argument.
If not given it will start with default parameter values and above made
`loadmodule` option is not effective. Refer to `redis-server --help`.

SDL API will check in connection setup phase that all required commands are
available, if not then execution is aborted and error log is written to identify
that which commands are missing.

## Running unit tests

Unit tests are compiled and executed by simply running:

    make test

By default all unit tests are executed. If `valgrind` is installed,
then by default unit test execution is analyzed with `valgrind`.
Running specific test cases can be achieved by using `GTEST_FILTER`
environment variable. For example:

    make test GTEST_FILTER=ConnectionTest*

If `valgrind` is not desired (even if installed), then it can be
disabled with `USE_VALGRIND` environment variable:

    make test USE_VALGRIND=false

Additional `valgrind` arguments can be specified with `VALGRIND_EXTRA_ARGS`
environment variable. For example:

    make test VALGRIND_EXTRA_ARGS='--track-fds=yes --log-file=mylog.txt'

It is also possible to use the `testrunner` binary directly (after it
has been compiled). The `testrunner` binary supports all the command
line options gtest supports, for example:

    make testrunner
    ./testrunner --help

## Running unit tests with gcov

Enable unit test gcov code coverage analysis by configuring gcov reporting
directory:

    ./configure --with-gcov-report-dir=DIR

Directory can be an absolute path or a relative path to an SDL source root.
Unit test build creates directory if it does not exist.

Build and run unit tests with code coverage analysis:

    make test_gcov

After successful unit test run code coverage (.gcov) result files are in
a directory, what was defined by '--with-gcov-report-dir' configure option.

In addition, graphical gcov front-ends such as lcov can be used for coverage
analysis:

    lcov --directory tst/ --directory src --capture --output-file coverage.info
    genhtml coverage.info --output-directory out

Open the out/index.html using any web browser.

## Docker Tests

It's also possible to test SDL compilation, run unit tests and test building of
rpm and Debian packages in a Docker:

    docker build  --no-cache -f docker_test/Dockerfile-Test -t sdltest:latest .

If needed, ready rpm and Debian packages can be copied from Docker to host. In
below example packages are copied to host's /tmp/sdltest-packages directory:

    docker run -v /tmp/sdltest-packages:/export sdltest:latest /export

## Using SDL in application pod

SDL is not yet available in O-RAN-SC PackageCloud.io repository.

Plan is to add it, but in the meantime SDL API needs to be complied and installed
in SDL client Dockerfiles (refer instructions above). You may need to execute
`RUN ldconfig` command to Dockerfile after SDL API installation.

RIC DBaaS service must be running before starting application pod which is using SDL
API. DBaaS defines environment variables which are used to contact DBaaS service
(offering backend for SDL). Those environment variables are exposed inside application
container only if DBaaS service is running when application container is started.

You may test SDL connectivity to its backend with `sdltool test-connectivity`
command inside your application container.
