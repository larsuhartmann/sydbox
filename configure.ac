dnl vim: set sw=4 sts=4 ts=4 noet ft=config foldmethod=marker foldmarker={{{,}}} :

dnl {{{ program, version
AC_PREREQ(2.59)
AC_INIT([src/main.c])
AC_CANONICAL_HOST

VERSION_MAJOR=0
VERSION_MINOR=1
VERSION_FULL="$VERSION_MAJOR.$VERSION_MINOR"
VERSION="$VERSION_FULL"

AC_SUBST([VERSION_MAJOR])
AC_SUBST([VERSION_MINOR])
AC_SUBST([VERSION_FULL])

AM_INIT_AUTOMAKE(sydbox, [$VERSION_FULL])

dnl {{{ git revision
AC_MSG_CHECKING([for git head])
if test -d "${GIT_DIR:-${ac_top_srcdir:-./}/.git}" ; then
	GITHEAD=`git describe 2>/dev/null`
	if test -z ${GITHEAD} ; then
		GITHEAD=`git rev-parse --short HEAD`
	fi
	if test -n "`git diff-index -m --name-only HEAD`" ; then
		GITHEAD=${GITHEAD}-dirty
	fi
fi
AC_MSG_RESULT([$GITHEAD])
AC_SUBST([GITHEAD])
dnl }}}
dnl }}}

dnl {{{ toolchain checks
AM_PROG_CC_C_O
AC_PROG_CC_C99
if test x"$ac_cv_prog_cc_c99" = x"no"; then
	AC_MSG_ERROR([sydbox requires a C compiler that supports ISO C99!])
fi
AC_PROG_INSTALL
AC_PROG_MAKE_SET
dnl }}}

dnl {{{ check for supported OS
AC_MSG_CHECKING([for supported OS])
case "$host_os" in
	linux*)
		AC_MSG_RESULT([$host_os])
		AC_MSG_CHECKING([for supported kernel])
		kernel_version=`uname -r | cut -d'-' -f1`
		case "$kernel_version" in
			2.6*)
				AC_MSG_RESULT([$kernel_version])
				;;
			2.5*)
				kernel_minor_version=`echo $kernel_version | cut -d'.' -f3`
				if test "$kernel_minor_version" -lt 46; then
					AC_MSG_RESULT([NO!])
					AC_MSG_ERROR([linux kernel version $kernel_version is not supported by sydbox])
				fi
				AC_MSG_RESULT([$kernel_version])
				;;
			*)
				AC_MSG_RESULT([NO!])
				AC_MSG_ERROR([linux kernel version $kernel_version is not supported by sydbox])
				;;
		esac
		;;
	*)
		AC_MSG_RESULT([NO!])
		AC_MSG_ERROR([operating system $host_os is not supported by sydbox])
		;;
esac
dnl }}}

dnl {{{ check for supported architecture
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

dnl {{{ Check for usable /dev/null
AC_MSG_CHECKING([for usable /dev/null])
AC_CACHE_VAL(ac_cv_dev_null_usable)
if ! test -c /dev/null; then
	AC_MSG_RESULT([NO!])
	AC_MSG_ERROR([/dev/null is not a special character file])
fi
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define DEVNULL "/dev/null"
#define TEST_STR "What exactly is a dream?"
#define TEST_LEN 25

int main(void) {
    int fd;
    char buf[TEST_LEN];

    fd = open(DEVNULL, O_WRONLY);
    if (0 > fd) {
        perror("open_wr");
        return EXIT_FAILURE;
    }

    if (0 > write(fd, TEST_STR, TEST_LEN)) {
        perror("write");
        return EXIT_FAILURE;
    }
    if (0 > close(fd)) {
        perror("close");
        return EXIT_FAILURE;
    }

    fd = open(DEVNULL, O_RDONLY);
    if (0 > fd) {
        perror("open_rd");
        return EXIT_FAILURE;
    }
    if (0 > read(fd, buf, TEST_LEN)) {
        perror("read");
        return EXIT_FAILURE;
    }
    if (0 == strncmp(buf, TEST_STR, TEST_LEN)) {
        fprintf(stderr, "strings match!\n");
        return EXIT_FAILURE;
    }
    if (0 > close(fd)) {
        perror("close");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
]])],
[AC_MSG_RESULT([yes]) ; ac_cv_dev_null_usable=yes],
[AC_MSG_RESULT([NO!]) ; ac_cv_dev_null_usable=no])
if test x"$ac_cv_dev_null_usable" = x"no"; then
	AC_MSG_ERROR([/dev/null fails to discard data])
fi
dnl }}}

dnl {{{ Check for usable /proc
AC_MSG_CHECKING(whether /proc/PID/cwd is usable)
AC_CACHE_VAL(ac_cv_proc_cwd_usable)
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    pid_t pid;


    pid = fork();

    if (0 > pid)
        exit(1);
    if (0 == pid) {
        pause();
        exit(0);
    }
    else {
        char proc[32], path[PATH_MAX];
        char *cwd = NULL;

        cwd = getcwd(cwd, PATH_MAX);
        if (NULL == cwd)
            goto fail;

        snprintf(proc, 32, "/proc/%i/cwd", pid);
        if (0 > readlink(proc, path, PATH_MAX))
            goto fail;
        if (0 != strncmp(cwd, path, PATH_MAX))
            goto fail;

        kill(pid, SIGKILL);
        exit(0);
fail:
        kill(pid, SIGKILL);
        exit(1);
    }
}
]])],
[AC_MSG_RESULT([yes]) ; ac_cv_proc_cwd_usable=yes],
[AC_MSG_RESULT([NO!]) ; ac_cv_proc_cwd_usable=no])
if test x"$ac_cv_proc_cwd_usable" = x"no"; then
	AC_MSG_ERROR([/proc fails to give current working directory information about pids])
fi

AC_MSG_CHECKING(whether /proc/PID/fd/DIRFD is usable)
AC_CACHE_VAL(ac_cv_proc_fd_usable)
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>

int main(void) {
    pid_t pid;
    char buf[32];
    int pfd[2];

    if (-1 == pipe(pfd))
        exit(1);

    pid = fork();
    if (0 == pid) {
        DIR *d = opendir("/dev");
        int dfd = dirfd(d);

        close(pfd[0]);

        snprintf(buf, 32, "%i", dfd);
        write(pfd[1], buf, 32);

        close(pfd[1]);

        pause();
        exit(0);
    }
    else {
        int dfd, n;
        char proc[64], path[PATH_MAX];

        close(pfd[1]);

        if (0 > read(pfd[0], buf, 32))
            goto fail;

        close(pfd[0]);

        dfd = atoi(buf);
        snprintf(proc, 64, "/proc/%i/fd/%i", pid, dfd);
        n = readlink(proc, path, PATH_MAX);
        if (0 > n)
            goto fail;
        path[n] = '\0';

        if (0 != strncmp(path, "/dev", 5))
            goto fail;

        kill(pid, SIGKILL);
        exit(0);
fail:
        kill(pid, SIGKILL);
        exit(1);
    }
}
]])],
[AC_MSG_RESULT([yes]) ; ac_cv_proc_fd_usable=yes],
[AC_MSG_RESULT([NO!]) ; ac_cv_proc_fd_usable=no])
if test x"$ac_cv_proc_fd_usable" = x"no"; then
	AC_MSG_ERROR([/proc fails to give directory file descriptor information about pids])
fi
dnl }}}

dnl {{{ Extra CFLAGS for gcc
SYDBOX_CFLAGS=
if test x"$GCC" = x"yes"; then
	SYDBOX_CFLAGS="$SYDBOX_CFLAGS -Wall -pedantic -D_GNU_SOURCE"
fi
AC_SUBST([SYDBOX_CFLAGS])
dnl }}}

dnl {{{ Check for libraries
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])
PKG_CHECK_MODULES([confuse], [libconfuse >= 2.6],,
				  [AC_MSG_ERROR([sydbox requires libconfuse >=2.6])])
dnl }}}

dnl {{{ output
AM_CONFIG_HEADER(config.h)
AC_OUTPUT(
	Makefile
	src/Makefile
	tests/Makefile
	tests/progtests/Makefile
	)
dnl }}}
