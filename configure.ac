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
m4_define([SDL_MINOR], [0])
m4_define([SDL_MICRO], [3])

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

m4_define([SDL_CURRENT], [1])
m4_define([SDL_REVISION], [3])
m4_define([SDL_AGE], [0])

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

DX_DOT_FEATURE(ON)
DX_INIT_DOXYGEN([shareddatalayer], [Doxyfile], [doxygen-doc])
SDL_LT_VERSION=m4_format("%d:%d:%d", SDL_CURRENT, SDL_REVISION, SDL_AGE)
AC_SUBST(SDL_LT_VERSION)
AC_OUTPUT
