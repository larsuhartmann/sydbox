dnl vim: set sw=4 sts=4 ts=4 noet ft=config foldmethod=marker foldmarker={{{,}}} :

dnl {{{ program, version
AC_PREREQ(2.59)
AC_INIT([src/main.c])

VERSION_MAJOR=0
VERSION_MINOR=1
VERSION_FULL="$VERSION_MAJOR.$VERSION_MINOR"
VERSION="$VERSION_FULL"

AC_SUBST([VERSION_MAJOR])
AC_SUBST([VERSION_MINOR])
AC_SUBST([VERSION_FULL])

AM_INIT_AUTOMAKE(sydbox, [$VERSION_FULL])
dnl }}}

dnl {{{ toolchain checks
AC_PROG_CC_C99
if test x"$ac_cv_prog_cc_c99" = x"no"; then
	AC_MSG_ERROR(["sydbox requires a C compiler that supports ISO C99!"])
fi
AC_PROG_INSTALL
AC_PROG_MAKE_SET
dnl }}}

dnl {{{ check for supported architecture
AC_CANONICAL_HOST
AC_MSG_CHECKING([for supported architecture])
case "$host_cpu" in
i[[3456]]86|pentium)
	arch=i386
	AC_DEFINE([I386], 1, [Define for the i386 architecture.])
	;;
x86?64*)
	arch=x86_64
	AC_DEFINE([X86_64], 1, [Define for the AMD x86-64 architecture.])
	;;
*)
	AC_MSG_RESULT([NO!])
	AC_MSG_ERROR([architecture $host_cpu is not supported by sydbox])
	;;
esac
AC_MSG_RESULT($arch)
dnl }}}

dnl {{{ Check headers
AC_CHECK_HEADERS([sys/reg.h], [], [])
dnl }}}

dnl {{{ Extra CFLAGS for gcc
SYDBOX_CFLAGS=
if test x"$GCC" = x"yes"; then
	SYDBOX_CFLAGS="$SYDBOX_CFLAGS -Wall -pedantic -D_GNU_SOURCE"
fi
AC_SUBST([SYDBOX_CFLAGS])
dnl }}}

dnl {{{ Tests
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])
dnl }}}

dnl {{{ output
AM_CONFIG_HEADER(config.h)
AC_OUTPUT(
	Makefile
	src/Makefile
	tests/Makefile
	)
dnl }}}
