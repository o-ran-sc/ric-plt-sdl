ACLOCAL_AMFLAGS = -I m4

BASE_CPPFLAGS = \
    -Wall \
    -Werror \
    -Wextra \
    -Wno-missing-field-initializers \
    -Wno-error=deprecated-declarations \
    -I$(top_srcdir)/include \
    -I$(builddir)/include \
    -DSDL_CONF_DIR='"@SDL_CONF_DIR@"' \
    $(BOOST_CPPFLAGS)

lib_LTLIBRARIES = \
    libsdl.la \
    libshareddatalayercli.la

libsdl_la_SOURCES = \
    include/private/abort.hpp \
    include/private/asyncconnection.hpp \
    include/private/asyncdummystorage.hpp \
    include/private/asyncstorageimpl.hpp \
    include/private/createlogger.hpp \
    include/private/configurationpaths.hpp \
    include/private/configurationreader.hpp \
    include/private/databaseconfiguration.hpp \
    include/private/databaseconfigurationimpl.hpp \
    include/private/engine.hpp \
    include/private/engineimpl.hpp \
    include/private/error.hpp \
    include/private/eventfd.hpp \
    include/private/filedescriptor.hpp \
    include/private/hostandport.hpp \
    include/private/logger.hpp \
    include/private/namespaceconfiguration.hpp \
    include/private/namespaceconfigurations.hpp \
    include/private/namespaceconfigurationsimpl.hpp \
    include/private/namespacevalidator.hpp \
    include/private/stdstreamlogger.hpp \
    include/private/syncstorageimpl.hpp \
    include/private/system.hpp \
    include/private/timer.hpp \
    include/private/timerfd.hpp \
    src/abort.cpp \
    src/asyncconnection.cpp \
    src/asyncdummystorage.cpp \
    src/asyncstorage.cpp \
    src/asyncstorageimpl.cpp \
    src/backenderror.cpp \
    src/configurationpaths.cpp \
    src/configurationreader.cpp \
    src/createlogger.cpp \
    src/databaseconfiguration.cpp \
    src/databaseconfigurationimpl.cpp \
    src/emptynamespace.cpp \
    src/engine.cpp \
    src/engineimpl.cpp \
    src/error.cpp \
    src/errorqueries.cpp \
    src/exception.cpp \
    src/eventfd.cpp \
    src/filedescriptor.cpp \
    src/hostandport.cpp \
    src/invalidnamespace.cpp \
    src/namespacevalidator.cpp \
    src/namespaceconfigurationsimpl.cpp \
    src/notconnected.cpp \
    src/operationinterrupted.cpp \
    src/publisherid.cpp \
    src/rejectedbybackend.cpp \
    src/rejectedbysdl.cpp \
    src/stdstreamlogger.cpp \
    src/syncstorage.cpp \
    src/syncstorageimpl.cpp \
    src/system.cpp \
    src/timer.cpp \
    src/timerfd.cpp

if SYSTEMLOGGER
libsdl_la_SOURCES += \
    include/private/systemlogger.hpp \
    src/systemlogger.cpp
endif

if REDIS
libsdl_la_SOURCES += \
    include/private/redis/asynccommanddispatcher.hpp \
    include/private/redis/asyncdatabasediscovery.hpp \
    include/private/redis/asyncredisreply.hpp \
    include/private/redis/asyncredisstorage.hpp \
    include/private/redis/asyncsentineldatabasediscovery.hpp \
    include/private/redis/contents.hpp \
    include/private/redis/contentsbuilder.hpp \
    include/private/redis/databaseinfo.hpp \
    include/private/redis/reply.hpp \
    src/redis/asynccommanddispatcher.cpp \
    src/redis/asyncdatabasediscovery.cpp \
    src/redis/asyncredisreply.cpp \
    src/redis/asyncredisstorage.cpp \
    src/redis/asyncsentineldatabasediscovery.cpp \
    src/redis/contentsbuilder.cpp
endif
if HIREDIS
libsdl_la_SOURCES += \
    include/private/redis/asynchirediscommanddispatcher.hpp \
    include/private/redis/asynchiredisdatabasediscovery.hpp \
    include/private/redis/hiredisepolladapter.hpp \
    include/private/redis/hiredissystem.hpp \
    include/private/redis/redisgeneral.hpp \
    src/redis/asynchirediscommanddispatcher.cpp \
    src/redis/asynchiredisdatabasediscovery.cpp \
    src/redis/hiredisepolladapter.cpp \
    src/redis/hiredissystem.cpp \
    src/redis/redisgeneral.cpp
endif
if HIREDIS_VIP
libsdl_la_SOURCES += \
    include/private/redis/asynchiredisclustercommanddispatcher.hpp \
    include/private/redis/hiredisclusterepolladapter.hpp \
    include/private/redis/hiredisclustersystem.hpp \
    src/redis/asynchiredisclustercommanddispatcher.cpp \
    src/redis/hiredisclusterepolladapter.cpp \
    src/redis/hiredisclustersystem.cpp
endif
libsdl_la_CPPFLAGS = \
    $(BASE_CPPFLAGS) \
    $(HIREDIS_CFLAGS) \
    $(HIREDIS_VIP_CFLAGS)
libsdl_la_LDFLAGS = \
    $(BOOST_LDFLAGS) \
    -version-info @SDL_LT_VERSION@
libsdl_la_LIBADD = \
    $(BOOST_SYSTEM_LIB) \
    $(BOOST_FILESYSTEM_LIB) \
    $(HIREDIS_LIBS) \
    $(HIREDIS_VIP_LIBS)

libshareddatalayercli_la_SOURCES = \
    src/cli/commandmap.cpp \
    src/cli/commandparserandexecutor.cpp \
    src/cli/dumpconfigurationcommand.cpp \
    src/cli/testgetsetcommand.cpp \
    src/cli/testconnectivitycommand.cpp \
    src/cli/listkeyscommand.cpp \
    src/cli/setcommand.cpp \
    src/cli/getcommand.cpp \
    src/exception.cpp \
    src/configurationpaths.cpp \
    include/private/configurationpaths.hpp \
    include/private/cli/commandmap.hpp \
    include/private/cli/commandparserandexecutor.hpp

libshareddatalayercli_la_CPPFLAGS = \
    $(BASE_CPPFLAGS) \
    $(BOOST_CPPFLAGS)
libshareddatalayercli_la_LDFLAGS = \
    $(BOOST_LDFLAGS) \
    -release $(PACKAGE_VERSION)
libshareddatalayercli_la_LIBADD = \
    $(BOOST_FILESYSTEM_LIB) \
    $(BOOST_PROGRAM_OPTIONS_LIB) \
    $(BOOST_SYSTEM_LIB) \
    -lboost_system \
    libsdl.la

pkgincludedir = \
    $(includedir)/sdl
pkginclude_HEADERS = \
    include/sdl/asyncstorage.hpp \
    include/sdl/backenderror.hpp \
    include/sdl/doxygen.hpp \
    include/sdl/emptynamespace.hpp \
    include/sdl/errorqueries.hpp \
    include/sdl/exception.hpp \
    include/sdl/invalidnamespace.hpp \
    include/sdl/notconnected.hpp \
    include/sdl/operationinterrupted.hpp \
    include/sdl/publisherid.hpp \
    include/sdl/rejectedbybackend.hpp \
    include/sdl/rejectedbysdl.hpp \
    include/sdl/syncstorage.hpp

pkgtstincludedir = \
    $(includedir)/sdl/tst
pkgtstinclude_HEADERS = \
    include/sdl/tst/mockableasyncstorage.hpp \
    include/sdl/tst/mockablesyncstorage.hpp

pkgconfigdir = \
    ${libdir}/pkgconfig
nodist_pkgconfig_DATA = \
    libsdl.pc

libsdl.pc: $(top_srcdir)/libsdl.pc.in
	sed -e 's![@]prefix[@]!$(prefix)!g' \
        -e 's![@]exec_prefix[@]!$(exec_prefix)!g' \
        -e 's![@]includedir[@]!$(includedir)!g' \
        -e 's![@]pkglibexecdir[@]!$(pkglibexecdir)!g' \
        -e 's![@]libdir[@]!$(libdir)!g' \
        -e 's![@]confdir[@]!@SDL_CONF_DIR@!g' \
        -e 's![@]PACKAGE_VERSION[@]!$(PACKAGE_VERSION)!g' \
        -e 's![@]PACKAGE_URL[@]!$(PACKAGE_URL)!g' \
        $< > $@

bin_PROGRAMS = \
    sdltool

sdltool_SOURCES = \
    src/cli/main.cpp
sdltool_CPPFLAGS = \
    $(BASE_CPPFLAGS) \
    $(BOOST_CPPFLAGS)
sdltool_LDFLAGS = \
    $(BOOST_LDFLAGS)
sdltool_LDADD = \
    $(BOOST_PROGRAM_OPTIONS_LIB) \
    libsdl.la \
    -lboost_system \
    libshareddatalayercli.la

check_LTLIBRARIES = \
    libgmock.la libgtest.la

libgmock_la_SOURCES = \
    3rdparty/googletest/googlemock/src/gmock-all.cc \
    3rdparty/googletest/googlemock/src/gmock-cardinalities.cc \
    3rdparty/googletest/googlemock/src/gmock.cc \
    3rdparty/googletest/googlemock/src/gmock-internal-utils.cc \
    3rdparty/googletest/googlemock/src/gmock_main.cc \
    3rdparty/googletest/googlemock/src/gmock-matchers.cc \
    3rdparty/googletest/googlemock/src/gmock-spec-builders.cc \
    3rdparty/googletest/googlemock/include/gmock/gmock-actions.h \
    3rdparty/googletest/googlemock/include/gmock/gmock-cardinalities.h \
    3rdparty/googletest/googlemock/include/gmock/gmock-generated-actions.h \
    3rdparty/googletest/googlemock/include/gmock/gmock-generated-function-mockers.h \
    3rdparty/googletest/googlemock/include/gmock/gmock-generated-matchers.h \
    3rdparty/googletest/googlemock/include/gmock/gmock-generated-nice-strict.h \
    3rdparty/googletest/googlemock/include/gmock/gmock.h \
    3rdparty/googletest/googlemock/include/gmock/gmock-matchers.h \
    3rdparty/googletest/googlemock/include/gmock/gmock-more-actions.h \
    3rdparty/googletest/googlemock/include/gmock/gmock-more-matchers.h \
    3rdparty/googletest/googlemock/include/gmock/gmock-spec-builders.h \
    3rdparty/googletest/googlemock/include/gmock/internal/gmock-generated-internal-utils.h \
    3rdparty/googletest/googlemock/include/gmock/internal/gmock-internal-utils.h \
    3rdparty/googletest/googlemock/include/gmock/internal/gmock-port.h \
    3rdparty/googletest/googlemock/include/gmock/internal/custom/gmock-generated-actions.h \
    3rdparty/googletest/googlemock/include/gmock/internal/custom/gmock-matchers.h \
    3rdparty/googletest/googlemock/include/gmock/internal/custom/gmock-port.h

libgmock_la_CPPFLAGS = \
    -I$(top_srcdir)/3rdparty/googletest/googlemock/include/ \
    -I$(top_srcdir)/3rdparty/googletest/googlemock/ \
    -I$(top_srcdir)/3rdparty/googletest/googletest/include/
libgmock_la_LIBADD = \
    -lpthread

libgtest_la_SOURCES = \
    3rdparty/googletest/googletest/src/gtest-all.cc \
    3rdparty/googletest/googletest/src/gtest.cc \
    3rdparty/googletest/googletest/src/gtest-death-test.cc \
    3rdparty/googletest/googletest/src/gtest-filepath.cc \
    3rdparty/googletest/googletest/src/gtest-internal-inl.h \
    3rdparty/googletest/googletest/src/gtest_main.cc \
    3rdparty/googletest/googletest/src/gtest-port.cc \
    3rdparty/googletest/googletest/src/gtest-printers.cc \
    3rdparty/googletest/googletest/src/gtest-test-part.cc \
    3rdparty/googletest/googletest/src/gtest-typed-test.cc \
    3rdparty/googletest/googletest/include/gtest/gtest-death-test.h \
    3rdparty/googletest/googletest/include/gtest/gtest.h \
    3rdparty/googletest/googletest/include/gtest/gtest-message.h \
    3rdparty/googletest/googletest/include/gtest/gtest-param-test.h \
    3rdparty/googletest/googletest/include/gtest/gtest-param-test.h.pump \
    3rdparty/googletest/googletest/include/gtest/gtest_pred_impl.h \
    3rdparty/googletest/googletest/include/gtest/gtest-printers.h \
    3rdparty/googletest/googletest/include/gtest/gtest_prod.h \
    3rdparty/googletest/googletest/include/gtest/gtest-spi.h \
    3rdparty/googletest/googletest/include/gtest/gtest-test-part.h \
    3rdparty/googletest/googletest/include/gtest/gtest-typed-test.h \
    3rdparty/googletest/googletest/include/gtest/internal/gtest-death-test-internal.h \
    3rdparty/googletest/googletest/include/gtest/internal/gtest-filepath.h \
    3rdparty/googletest/googletest/include/gtest/internal/gtest-internal.h \
    3rdparty/googletest/googletest/include/gtest/internal/gtest-linked_ptr.h \
    3rdparty/googletest/googletest/include/gtest/internal/gtest-param-util-generated.h \
    3rdparty/googletest/googletest/include/gtest/internal/gtest-param-util.h \
    3rdparty/googletest/googletest/include/gtest/internal/gtest-port-arch.h \
    3rdparty/googletest/googletest/include/gtest/internal/gtest-port.h \
    3rdparty/googletest/googletest/include/gtest/internal/gtest-string.h \
    3rdparty/googletest/googletest/include/gtest/internal/gtest-tuple.h \
    3rdparty/googletest/googletest/include/gtest/internal/gtest-type-util.h \
    3rdparty/googletest/googletest/include/gtest/internal/custom/gtest.h \
    3rdparty/googletest/googletest/include/gtest/internal/custom/gtest-port.h \
    3rdparty/googletest/googletest/include/gtest/internal/custom/gtest-printers.h

libgtest_la_CPPFLAGS = \
    -I$(top_srcdir)/3rdparty/googletest/googletest/include/ \
    -I$(top_srcdir)/3rdparty/googletest/googletest/
libgtest_la_LIBADD = \
    -lpthread

check_PROGRAMS = \
    testrunner

testrunner_SOURCES = \
    include/private/tst/asyncstoragemock.hpp \
    include/private/tst/databaseconfigurationmock.hpp \
    include/private/tst/enginemock.hpp \
    include/private/tst/gettopsrcdir.hpp \
    include/private/tst/namespaceconfigurationsmock.hpp \
    include/private/tst/syncstorageimplmock.hpp \
    include/private/tst/systemmock.hpp \
    include/private/tst/wellknownerrorcode.hpp \
    tst/abort_test.cpp \
    tst/asyncdummystorage_test.cpp \
    tst/asyncstorage_test.cpp \
    tst/backenderror_test.cpp \
    tst/configurationreader_test.cpp \
    tst/databaseconfiguration_test.cpp \
    tst/databaseconfigurationimpl_test.cpp \
    tst/emptynamespace_test.cpp \
    tst/engine_test.cpp \
    tst/engineimpl_test.cpp \
    tst/error_test.cpp \
    tst/errorqueries_test.cpp \
    tst/eventfd_test.cpp \
    tst/gettopsrcdir.cpp \
    tst/filedescriptor_test.cpp \
    tst/hostandport_test.cpp \
    tst/invalidnamespace_test.cpp \
    tst/main.cpp \
    tst/mockableasyncstorage_test.cpp \
    tst/mockablesyncstorage_test.cpp \
    tst/namespaceconfigurations_test.cpp \
    tst/namespaceconfigurationsimpl_test.cpp \
    tst/namespacevalidator_test.cpp \
    tst/publisherid_test.cpp \
    tst/syncstorage_test.cpp \
    tst/syncstorageimpl_test.cpp \
    tst/system_test.cpp \
    tst/timer_test.cpp \
    tst/timerfd_test.cpp \
    tst/wellknownerrorcode.cpp
if REDIS
testrunner_SOURCES += \
    include/private/tst/asynccommanddispatchermock.hpp \
    include/private/tst/asyncdatabasediscoverymock.hpp \
    include/private/tst/contentsbuildermock.hpp \
    include/private/tst/redisreplybuilder.hpp \
    include/private/tst/replymock.hpp \
    tst/asynccommanddispatcher_test.cpp \
    tst/asyncdatabasediscovery_test.cpp \
    tst/asyncredisstorage_test.cpp \
    tst/asyncsentineldatabasediscovery_test.cpp \
    tst/asyncstorageimpl_test.cpp \
    tst/contents_test.cpp \
    tst/contentsbuilder_test.cpp \
    tst/databaseinfo_test.cpp \
    tst/redisgeneral_test.cpp \
    tst/redisreplybuilder.cpp \
    tst/reply_test.cpp
endif
if HIREDIS
testrunner_SOURCES += \
    include/private/tst/hiredisepolladaptermock.hpp \
    include/private/tst/hiredissystemmock.hpp \
    tst/asynchirediscommanddispatcher_test.cpp \
    tst/asynchiredisdatabasediscovery_test.cpp \
    tst/hiredisepolladapter_test.cpp \
    tst/hiredissystem_test.cpp
endif
if HIREDIS_VIP
testrunner_SOURCES += \
    include/private/tst/hiredisclusterepolladaptermock.hpp \
    include/private/tst/hiredisclustersystemmock.hpp \
    tst/asynchiredisclustercommanddispatcher_test.cpp \
    tst/hiredisclusterepolladapter_test.cpp \
    tst/hiredisclustersystem_test.cpp
endif
testrunner_CPPFLAGS = \
    $(BASE_CPPFLAGS) \
    -I$(top_srcdir)/3rdparty/googletest/googlemock/include \
    -I$(top_srcdir)/3rdparty/googletest/googletest/include \
    $(HIREDIS_CFLAGS) \
    $(HIREDIS_VIP_CFLAGS)
testrunner_LDADD = \
    libsdl.la \
    libgmock.la \
    libgtest.la

TESTS = \
    run-tests.sh

test: testrunner
	./run-tests.sh

if ENABLE_GCOV
AM_CXXFLAGS= -O0 --coverage
AM_LDFLAGS= --coverage

test_gcov: test
	@for p in src/.libs/*.o src/redis/.libs/*.o; do \
		gcov -abcfru $$p 1>/dev/null; \
	done
	mkdir -p @GCOV_REPORT_DIR@
	mv *.gcov @GCOV_REPORT_DIR@
	ls -la @GCOV_REPORT_DIR@
endif

if ENABLE_RPM_BUILD
rpm-pkg:
	rpmbuild --nodeps --nocheck -bb rpm/sdl.spec --define="_sourcedir $(top_srcdir)" --define="_builddir $(top_srcdir)" --define="_rpmdir .."
	mkdir -p @RPM_DIR@
	cp ../x86_64/*.rpm @RPM_DIR@
endif


if ENABLE_DEB_BUILD
deb-pkg:
	DEB_BUILD_OPTIONS='nocheck noddebs' debuild -b -us -uc
	mkdir -p @DEB_DIR@
	cp ../*.deb @DEB_DIR@
endif

clean-local:
	rm -f libsdl.pc
if ENABLE_GCOV
	rm -rf @GCOV_REPORT_DIR@
	@find . -name '*.gcno' -o -name '*.gcda'| xargs -r rm
endif

@DX_RULES@

MOSTLYCLEANFILES = \
    $(DX_CLEANFILES)

EXTRA_DIST = \
    $(top_srcdir)/libsdl.pc.in \
    $(top_srcdir)/tst/valgrind-suppressions.conf \
    $(top_srcdir)/LICENSES.txt

AUTOMAKE_OPTIONS = \
    serial-tests
