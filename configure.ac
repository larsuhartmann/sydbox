dnl vim: set sw=4 sts=4 ts=4 noet ft=config foldmethod=marker foldmarker={{{,}}} :

dnl {{{ program, version
AC_PREREQ(2.59)
AC_INIT([src/main.c])
AC_CANONICAL_HOST

VERSION_MAJOR=0
VERSION_MINOR=1
VERSION_SUFFIX=_rc4
VERSION_FULL="$VERSION_MAJOR.$VERSION_MINOR$VERSION_SUFFIX"
VERSION="$VERSION_FULL"

AC_SUBST([VERSION_MAJOR])
AC_SUBST([VERSION_MINOR])
AC_SUBST([VERSION_FULL])

AM_INIT_AUTOMAKE(sydbox, [$VERSION_FULL])

dnl {{{ git revision
AC_MSG_CHECKING([for git head])
if test -d "${GIT_DIR:-${ac_top_srcdir:-./}/.git}" ; then
	GIT_HEAD=`git describe 2>/dev/null`
	if test -z ${GIT_HEAD} ; then
		GIT_HEAD=`git rev-parse --short HEAD`
	fi
	if test -n "`git diff-index -m --name-only HEAD`" ; then
		GIT_HEAD=${GIT_HEAD}-dirty
	fi
	if test -n "${GIT_HEAD}" ; then
		GIT_HEAD="-${GIT_HEAD}"
	fi
fi
AC_MSG_RESULT([$GIT_HEAD])
AC_SUBST([GIT_HEAD])
dnl }}}
dnl }}}

dnl {{{ toolchain checks
AM_PROG_CC_C_O
AC_PROG_CC_C99
if test x"$ac_cv_prog_cc_c99" = x"no"; then
	AC_MSG_ERROR([sydbox requires a C compiler that supports ISO C99!])
fi
AC_GNU_SOURCE
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_SED
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
				kernel_minor_version=`echo $kernel_version | cut -d'.' -f3`
				if test "$kernel_minor_version" -lt 29; then
					AC_MSG_WARN([sydbox works slow on kernels <2.6.29 due to a ptrace bug!])
					AC_MSG_WARN([See http://git.kernel.org/?p=linux/kernel/git/torvalds/linux-2.6.git;a=commit;h=53da1d9456fe7f87a920a78fdbdcf1225d197cb7 for the fix!])
				fi
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
ia64)
	arch=ia64
	AC_DEFINE([IA64], 1, [Define for the IA64 architecture.])
	;;
powerpc*)
	arch=powerpc
	AC_DEFINE([POWERPC], 1, [Define for the POWERPC architecture.])
	;;
*)
	AC_MSG_RESULT([NO!])
	AC_MSG_ERROR([architecture $host_cpu is not supported by sydbox])
	;;
esac
AC_MSG_RESULT($arch)
AM_CONDITIONAL(I386, test x"$arch" = x"i386")
AM_CONDITIONAL(X86_64, test x"$arch" = x"x86_64")
AM_CONDITIONAL(IA64, test x"$arch" = x"ia64")
AM_CONDITIONAL(POWERPC, test x"$arch" = x"powerpc")
AM_CONDITIONAL(P1, test x"$arch" = x"i386" -o x"$arch" = x"ia64" -o x"$arch" = x"powerpc")
AM_CONDITIONAL(P2, test x"$arch" = x"x86_64")
dnl }}}

dnl {{{ Check headers
AC_CHECK_HEADERS([asm/reg.h sys/reg.h], [], [])
dnl }}}

dnl {{{ Check functions
AC_CHECK_FUNCS([fchdir])
dnl }}}

dnl {{{ Check types
AC_CHECK_TYPES([struct pt_all_user_regs, struct ia64_fpreg],,,
			   [#include <sys/ptrace.h>])
dnl }}}

dnl {{{ Check whether getcwd(NULL, 0) allocates memory for result
AC_MSG_CHECKING([whether getcwd(NULL, 0) allocates memory for result])
AC_CACHE_VAL([ac_cv_func_getcwd_null])
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    if (0 != chdir("/"))
        return EXIT_FAILURE;
    else {
        char *f = getcwd(NULL, 0);
        if (NULL != f && '/' == f[0] && '\0' == f[1])
            return EXIT_SUCCESS;
        else
            return EXIT_FAILURE;
    }
}
]])],
[AC_MSG_RESULT([yes]) ; ac_cv_func_getcwd_null=yes],
[AC_MSG_RESULT([no]) ; ac_cv_func_getcwd_null=no])
if test x"$ac_cv_func_getcwd_null" = x"yes"; then
	AC_DEFINE([HAVE_GETCWD_NULL], 1, [Define for getcwd(NULL, 0) that allocates memory])
fi
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
        _exit(0);
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
        _exit(0);
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

AC_MSG_CHECKING([whether /proc/self resolves to /proc/PID])
AC_CACHE_VAL(ac_cv_have_prof_self)
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define PROC_SELF "/proc/self"

int main(void) {
    char buf[64], proc_pid[64];
    pid_t pid;

    if (0 > readlink(PROC_SELF, buf, 64)) {
        perror("readlink");
        return EXIT_FAILURE;
    }

    pid = getpid();
    snprintf(proc_pid, 64, "%i", pid);
    return (0 == strncmp(buf, proc_pid, 64)) ? EXIT_SUCCESS : EXIT_FAILURE;
}
]])],
[AC_MSG_RESULT([yes]) ; ac_cv_have_proc_self=yes],
[AC_MSG_RESULT([no]) ; ac_cv_have_proc_self=no])
if test x"$ac_cv_have_proc_self" = x"yes"; then
	AC_DEFINE([HAVE_PROC_SELF], 1, [Define for /proc/self])
fi
dnl }}}

dnl {{{ Extra CFLAGS
SYDBOX_CFLAGS=
WANTED_CFLAGS="-pedantic -Wall -W -Wextra -Wbad-function-cast -Wcast-align -Wcast-qual -Wfloat-equal -Wformat=2 -Wformat-security -Wformat-nonliteral -Winit-self -Winline -Wlogical-op -Wmissing-prototypes -Wmissing-declarations -Wmissing-format-attribute -Wmissing-noreturn -Wpointer-arith -Wredundant-decls -Wshadow -Wswitch-default -Wunused -Wvla"
for flag in $WANTED_CFLAGS ; do
	AX_CHECK_COMPILER_FLAGS([$flag], [SYDBOX_CFLAGS="$SYDBOX_CFLAGS $flag"],)
done
dnl }}}

dnl {{{ Check for gcc
AM_CONDITIONAL(GCC, test x"$GCC" = x"yes")
dnl }}}

dnl {{{ Profiling
AC_MSG_CHECKING([whether gprof symbols are wanted])
AC_ARG_ENABLE([gprof],
			  [AS_HELP_STRING([--enable-gprof],
							  [add gprof symbols (-pg) (for debugging)])],
			  SYDBOX_PROFILE="$enableval",
			  SYDBOX_PROFILE="no")
AC_MSG_RESULT([$SYDBOX_PROFILE])
if test x"$SYDBOX_PROFILE" = x"yes" ; then
	AX_CHECK_COMPILER_FLAGS([-pg],, AC_MSG_ERROR([your compiler doesn't support -pg flag]))
	SYDBOX_CFLAGS="$SYDBOX_CFLAGS -pg"
	LDFLAGS="$LDFLAGS -pg"
fi
AC_SUBST([SYDBOX_CFLAGS])
dnl }}}

dnl {{{ Check for libraries
GLIB_REQUIRED=2.0
CHECK_REQUIRED=0.9.4

PKG_PROG_PKG_CONFIG([0.20.0])
PKG_CHECK_MODULES([glib], [glib-2.0 >= $GLIB_REQUIRED],,
				  AC_MSG_ERROR([sydbox requires glib-$GLIB_REQUIRED or newer]))
PKG_CHECK_MODULES([gobject], [gobject-2.0 >= $GLIB_REQUIRED],,
				  AC_MSG_ERROR([sydbox requires gobject-$GLIB_REQUIRED or newer]))
PKG_CHECK_MODULES([check], [check >= $CHECK_REQUIRED])
dnl }}}

dnl {{{ Check for Perl
AC_PATH_PROG([PERL], perl)
dnl }}}

dnl {{{ Use AM_SILENT_RULES if present, shave otherwise
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES(yes)], [SHAVE_INIT([.], [enable])])
dnl }}}

dnl {{{ output
AM_CONFIG_HEADER(config.h)
AC_OUTPUT(
	shave
	Makefile
	data/Makefile
	src/Makefile
	tests/Makefile
	tests/progtests/Makefile
	tests/unit/Makefile
	)
dnl }}}

