/* Sydbox testcases for trace.c
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#define _AT_FILE_SOURCE

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <asm/unistd.h>
#include <sys/ptrace.h>

#include <check.h>

#include "../src/defs.h"
#include "check_sydbox.h"

START_TEST(check_ptrace_get_syscall) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        open("/dev/null", O_RDONLY);
    }
    else { /* parent */
        int status, syscall;
        struct tchild *tc = NULL;

        tchild_new(&tc, pid);
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        tchild_setup(tc);

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        syscall = ptrace_get_syscall(pid);
        fail_unless(__NR_open == syscall,
                "Expected __NR_open, got %d", syscall);

        tchild_free(&tc);
        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_ptrace_set_syscall) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        open("/dev/null", O_RDONLY);
    }
    else { /* parent */
        int status, syscall;
        struct tchild *tc = NULL;

        tchild_new(&tc, pid);
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        tchild_setup(tc);

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        ptrace_set_syscall(pid, 0xbadca11);

        /* Resume the child, it will stop at the end of the system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        syscall = ptrace_get_syscall(pid);
        fail_unless(0xbadca11 == syscall,
                "Expected 0xbadca11, got %d", syscall);

        tchild_free(&tc);
        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_ptrace_get_string_first) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        open("/dev/null", O_RDONLY);
    }
    else { /* parent */
        int status;
        char pathname[PATH_MAX];
        struct tchild *tc = NULL;

        tchild_new(&tc, pid);
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        tchild_setup(tc);

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        ptrace_get_string(pid, 1, pathname, PATH_MAX);
        fail_unless(0 == strncmp(pathname, "/dev/null", 10),
                "Expected '/dev/null' got '%s'", pathname);

        tchild_free(&tc);
        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_ptrace_get_string_second) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        openat(AT_FDCWD, "/dev/null", O_RDONLY);
    }
    else { /* parent */
        int status;
        char pathname[PATH_MAX];
        struct tchild *tc = NULL;

        tchild_new(&tc, pid);
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        tchild_setup(tc);

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        ptrace_get_string(pid, 2, pathname, PATH_MAX);
        fail_unless(0 == strncmp(pathname, "/dev/null", 10),
                "Expected '/dev/null' got '%s'", pathname);

        tchild_free(&tc);
        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_ptrace_get_string_third) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        symlinkat("emily", AT_FDCWD, "arnold_layne");
    }
    else { /* parent */
        int status;
        char pathname[PATH_MAX];
        struct tchild *tc = NULL;

        tchild_new(&tc, pid);
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        tchild_setup(tc);

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        ptrace_get_string(pid, 3, pathname, PATH_MAX);
        fail_unless(0 == strncmp(pathname, "arnold_layne", 13),
                "Expected 'arnold_layne' got '%s'", pathname);

        tchild_free(&tc);
        kill(pid, SIGTERM);
    }
}
END_TEST

/* FIXME why does this fail? */
#if 0
START_TEST(check_ptrace_get_string_fourth) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        linkat(AT_FDCWD, "emily", AT_FDCWD, "arnold_layne", 0600);
    }
    else { /* parent */
        int status;
        char pathname[PATH_MAX];
        struct tchild *tc = NULL;

        tchild_new(&tc, pid);
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        tchild_setup(tc);

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        ptrace_get_string(pid, 4, pathname, PATH_MAX);
        fail_unless(0 == strncmp(pathname, "arnold_layne", 13),
                "Expected 'arnold_layne' got '%s'", pathname);

        tchild_free(&tc);
        kill(pid, SIGTERM);
    }
}
END_TEST
#endif

START_TEST(check_ptrace_set_string_first) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        open("/dev/null", O_RDONLY);
    }
    else { /* parent */
        int status;
        char pathname[PATH_MAX];
        struct tchild *tc = NULL;

        tchild_new(&tc, pid);
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        tchild_setup(tc);

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        ptrace_set_string(pid, 1, "/dev/zero", 10);
        ptrace_get_string(pid, 1, pathname, PATH_MAX);
        fail_unless(0 == strncmp(pathname, "/dev/zero", 10),
                "Expected '/dev/zero' got '%s'", pathname);

        tchild_free(&tc);
        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_ptrace_set_string_second) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        openat(AT_FDCWD, "/dev/null", O_RDONLY);
    }
    else { /* parent */
        int status;
        char pathname[PATH_MAX];
        struct tchild *tc = NULL;

        tchild_new(&tc, pid);
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        tchild_setup(tc);

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        ptrace_set_string(pid, 2, "/dev/zero", 10);
        ptrace_get_string(pid, 2, pathname, PATH_MAX);
        fail_unless(0 == strncmp(pathname, "/dev/zero", 10),
                "Expected '/dev/zero' got '%s'", pathname);

        tchild_free(&tc);
        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_ptrace_set_string_third) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        symlinkat("emily", AT_FDCWD, "arnold_layne");
    }
    else { /* parent */
        int status;
        char pathname[PATH_MAX];
        struct tchild *tc = NULL;

        tchild_new(&tc, pid);
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        tchild_setup(tc);

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        ptrace_set_string(pid, 3, "its_not_the_same", 17);
        ptrace_get_string(pid, 3, pathname, PATH_MAX);
        fail_unless(0 == strncmp(pathname, "its_not_the_same", 17),
                "Expected 'its_not_the_same' got '%s'", pathname);

        tchild_free(&tc);
        kill(pid, SIGTERM);
    }
}
END_TEST


Suite *trace_suite_create(void) {
    Suite *s = suite_create("trace");

    /* ptrace_* test cases */
    TCase *tc_ptrace = tcase_create("ptrace");
    tcase_add_test(tc_ptrace, check_ptrace_get_syscall);
    tcase_add_test(tc_ptrace, check_ptrace_set_syscall);
    tcase_add_test(tc_ptrace, check_ptrace_get_string_first);
    tcase_add_test(tc_ptrace, check_ptrace_get_string_second);
    tcase_add_test(tc_ptrace, check_ptrace_get_string_third);
#if 0
    tcase_add_test(tc_ptrace, check_ptrace_get_string_fourth);
#endif
    tcase_add_test(tc_ptrace, check_ptrace_set_string_first);
    tcase_add_test(tc_ptrace, check_ptrace_set_string_second);
    tcase_add_test(tc_ptrace, check_ptrace_set_string_third);
    suite_add_tcase(s, tc_ptrace);

    return s;
}
