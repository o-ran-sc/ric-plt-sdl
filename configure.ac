# SDL version
#
# 1. If no interface changes has been made, increment SDL_MICRO.
# 2. Else, if new interfaces have been added but existing interfaces have not
#    been changed, increment SDL_MINOR and zero SDL_MICRO. (Backward compatible
#    interface change.)
# 3. Else, increment SDL_MAJOR and zero both SDL_MINOR and SDL_MICRO. (Backward
#    incompatible interface change.)
#
# Change the numbers just before release.

m4_define([SDL_MAJOR], [1])
m4_define([SDL_MINOR], [2])
m4_define([SDL_MICRO], [1])

# SDL ABI version with libtool
#
# 1. Always increase the revision value.
# 2. Increase the current value whenever an interface has been added, removed
#    or changed.
# 3. Increase the age value only if the changes made to the ABI are backward
#    compatible.
#
# Never zero or decrease any of the ABI version number values.
#
# See https://autotools.io/libtool/version.html
#
# Change the numbers just before release.

m4_define([SDL_CURRENT], [2])
m4_define([SDL_REVISION], [8])
m4_define([SDL_AGE], [1])

AC_INIT([shareddatalayer], [SDL_MAJOR.SDL_MINOR.SDL_MICRO], [], [], [https://gerrit.o-ran-sc.org/r/admin/repos/ric-plt/sdl])
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([Doxyfile])
AC_CONFIG_FILES([rpm/sdl.spec])
AC_CONFIG_FILES([debian/changelog])
AC_CONFIG_FILES([run-tests.sh], [chmod +x run-tests.sh])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([subdir-objects foreign 1.13 tar-pax -Wall -Werror])
AM_PROG_AR
LT_INIT
AM_SILENT_RULES([yes])
AC_PROG_CXX
AC_LANG([C++])
AC_PROG_MKDIR_P
AM_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])
AX_CXX_COMPILE_STDCXX([11], [noext], [mandatory])
AX_BOOST_BASE([1.53], [], [AC_MSG_ERROR(Unable to detect suitable boost library)])
AX_BOOST_PROGRAM_OPTIONS
AX_BOOST_SYSTEM
AX_BOOST_FILESYSTEM
PKG_PROG_PKG_CONFIG

AC_DEFINE([HAVE_SYSTEMLOGGER], [0], [Have systemlogger])
AM_CONDITIONAL([SYSTEMLOGGER], [test "xyes" = "xno"])

SDL_CONF_DIR=$sysconfdir/$PACKAGE_NAME.d
AC_SUBST(SDL_CONF_DIR)

AC_DEFINE(HAVE_REDIS, [1], [Have redis])
AM_CONDITIONAL([REDIS], [test xtrue])

PKG_CHECK_MODULES([HIREDIS], [hiredis])
AC_DEFINE(HAVE_HIREDIS, [1], [Have hiredis])
AM_CONDITIONAL([HIREDIS], [test xtrue])
# Redis cluster configuration is not supported in RIC currently (implementation is not compatible with current hiredis-vip version)
AC_DEFINE(HAVE_HIREDIS_VIP, [0], [Have hiredis-vip])
AM_CONDITIONAL([HIREDIS_VIP], [test "xyes" = "xno"])

#
# Configuration option --with-rpm-dir=DIR
#   If this option is given, rpm packages building is enabled and ready rpms are
#   copied to the directory.
#
AC_ARG_WITH([rpm-dir],
    AS_HELP_STRING([--with-rpm-dir=DIR],
        [Directory for build rpm packages]),
    [AC_CHECK_PROG(RPMBUILD, rpmbuild, "yes", "no")
     AC_CHECK_PROG(RPM, rpm, "yes", "no")
     AS_IF([test "x$RPMBUILD" != xyes],
         [AC_MSG_ERROR([rpmbuild needs to be installed])],
     [test "x$RPM" != xyes],
         [AC_MSG_ERROR([rpm is needs to be installed])])],
    [with_rpm_dir=no])

AC_MSG_CHECKING([rpm package])
if test "x$with_rpm_dir" = "xno"; then
    AC_MSG_RESULT([no])
    RPM_DIR="/tmp"
else
    AC_MSG_RESULT([yes])
    AC_MSG_NOTICE([rpm directory: $with_rpm_dir])
    RPM_DIR="$with_rpm_dir"
fi
AC_SUBST(RPM_DIR)
AM_CONDITIONAL([ENABLE_RPM_BUILD],[test "x$with_rpm_dir" != "xno"])


#
# Configuration option --with-deb-dir=DIR
#   If this option is given, Debian packages packages building is enabled and ready
#   packages are copied to the directory.
#
AC_ARG_WITH([deb-dir],
    AS_HELP_STRING([--with-deb-dir=DIR],
        [Directory for build Debian packages]),
    [AC_CHECK_PROG(DEBUILD, dpkg-buildpackage, "yes", "no")
     AS_IF([test "x$DEBUILD" != xyes],
         [AC_MSG_ERROR([dpkg-buildpackage needs to be installed])],
             [test "x$with_rpm_dir" != "xno"],
			     [AC_MSG_ERROR([--with-deb-dir and --with-rpm-dir are mutually exclusive])])],
    [with_deb_dir=no])

AC_MSG_CHECKING([deb package])
if test "x$with_deb_dir" = "xno"; then
    AC_MSG_RESULT([no])
    DEB_DIR="/tmp"
else
    AC_MSG_RESULT([yes])
    AC_MSG_NOTICE([deb directory: $with_deb_dir])
    DEB_DIR="$with_deb_dir"
fi
AC_SUBST(DEB_DIR)
AM_CONDITIONAL([ENABLE_DEB_BUILD],[test "x$with_deb_dir" != "xno"])


#
# Configuration option --with-gcov-report-dir=DIR
#   If this option is given, gcov unit test coverage analysis is enabled and
#   results are copied to the directory.
#
AC_ARG_WITH([gcov-report-dir],
    AS_HELP_STRING([--with-gcov-report-dir=DIR],
        [Directory for GCOV report files]),
    [],
    [with_gcov_report_dir=no])
AC_MSG_CHECKING([gcov])
if test "x$with_gcov_report_dir" = "xno"; then
    AC_MSG_RESULT([no])
    GCOV_REPORT_DIR="gcov_report"
else
    AC_MSG_RESULT([yes])
    AC_MSG_NOTICE([gcov report directory: $with_gcov_report_dir])
    GCOV_REPORT_DIR="$with_gcov_report_dir"
fi
AC_SUBST(GCOV_REPORT_DIR)
AM_CONDITIONAL([ENABLE_GCOV],[test "x$with_gcov_report_dir" != "xno"])

DX_DOT_FEATURE(ON)
DX_INIT_DOXYGEN([shareddatalayer], [Doxyfile], [doxygen-doc])
SDL_LT_VERSION=m4_format("%d:%d:%d", SDL_CURRENT, SDL_REVISION, SDL_AGE)
AC_SUBST(SDL_LT_VERSION)
AC_OUTPUT
