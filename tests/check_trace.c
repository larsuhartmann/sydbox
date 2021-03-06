/* Sydbox testcases for trace.c
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#define _ATFILE_SOURCE

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
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

#include "../src/trace.h"
#include "../src/children.h"
#include "check_sydbox.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#if defined(I386) || defined(IA64) || defined(POWERPC)
#define CHECK_PERSONALITY 0
#elif defined(X86_64)
#define CHECK_PERSONALITY 1
#else
#error unsupported architecture
#endif

void trace_teardown(void) {
    unlink("david");
    unlink("arnold_layne");
    unlink("its_not_the_same");
}

START_TEST(check_trace_event_e_stop) {
    PRINT_TEST_HEADER();
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
    }
    else { /* parent */
        int ret, status;

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        ret = trace_event(status);
        fail_unless(E_STOP == ret, "Expected E_SETUP got %d", ret);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_trace_event_e_syscall) {
    PRINT_TEST_HEADER();
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        sleep(1);
    }
    else { /* parent */
        int ret, status;

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        /* Check the event */
        ret = trace_event(status);
        fail_unless(E_SYSCALL == ret, "Expected E_SYSCALL got %d", ret);

        kill(pid, SIGTERM);
    }
}
END_TEST

/* TODO
 * START_TEST(check_trace_event_e_fork)
 * START_TEST(check_trace_event_e_vfork)
 * START_TEST(check_trace_event_e_clone)
 * START_TEST(check_trace_event_e_execv)
 */

START_TEST(check_trace_event_e_genuine) {
    PRINT_TEST_HEADER();
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        kill(getpid(), SIGINT);
    }
    else { /* parent */
        int ret, status;

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will receive a SIGINT */
        fail_if(0 > trace_cont(pid), "trace_cont() failed: %s", strerror(errno));
        wait(&status);

        /* Check the event */
        ret = trace_event(status);
        fail_unless(E_GENUINE == ret, "Expected E_GENUINE got %d", ret);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_trace_event_e_exit) {
    PRINT_TEST_HEADER();
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        exit(EXIT_SUCCESS);
    }
    else { /* parent */
        int ret, status;

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will exit. */
        fail_if(0 > trace_cont(pid), "trace_cont() failed: %s", strerror(errno));
        wait(&status);

        /* Check the event */
        ret = trace_event(status);
        fail_unless(E_EXIT == ret, "Expected E_EXIT got %d", ret);
    }
}
END_TEST

START_TEST(check_trace_event_e_exit_signal) {
    PRINT_TEST_HEADER();
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        for(;;)
            sleep(1);
    }
    else { /* parent */
        int ret, status;

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child. */
        fail_if(0 > trace_cont(pid), "trace_cont() failed: %s", strerror(errno));
        /* Kill it with a signal. */
        kill(pid, SIGKILL);
        wait(&status);

        /* Check the event */
        ret = trace_event(status);
        fail_unless(E_EXIT_SIGNAL == ret, "Expected E_EXIT_SIGNAL got %d", ret);
    }
}
END_TEST

START_TEST(check_trace_get_syscall) {
    PRINT_TEST_HEADER();
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        open("/dev/null", O_RDONLY);
        pause();
    }
    else { /* parent */
        int status;
        long sno;

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(__NR_open == sno, "Expected __NR_open, got %d", sno);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_trace_set_syscall) {
    PRINT_TEST_HEADER();
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        open("/dev/null", O_RDONLY);
    }
    else { /* parent */
        int status;
        long sno;

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_set_syscall(pid, 0xbadca11), "Failed to set syscall: %s", strerror(errno));

        /* Resume the child, it will stop at the end of the system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_unless(0 == trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(0xbadca11 == sno, "Expected 0xbadca11, got %d", sno);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_trace_get_path_first) {
    PRINT_TEST_HEADER();
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        open("/dev/null", O_RDONLY);
    }
    else { /* parent */
        int status;
        char *path;

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        path = trace_get_path(pid, CHECK_PERSONALITY, 0);
        fail_if(NULL == path, "Failed to get string: %s", strerror(errno));
        fail_unless(0 == strncmp(path, "/dev/null", 10), "Expected '/dev/null' got '%s'", path);

        free(path);
        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_trace_get_path_second) {
    PRINT_TEST_HEADER();
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        openat(AT_FDCWD, "/dev/null", O_RDONLY);
    }
    else { /* parent */
        int status;
        char *path;

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        path = trace_get_path(pid, CHECK_PERSONALITY, 1);
        fail_if(NULL == path, "Failed to get string: %s", strerror(errno));
        fail_unless(0 == strncmp(path, "/dev/null", 10), "Expected '/dev/null' got '%s'", path);

        free(path);
        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_trace_get_path_third) {
    PRINT_TEST_HEADER();
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        symlinkat("emily", AT_FDCWD, "arnold_layne");
    }
    else { /* parent */
        int status;
        char *path;

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        path = trace_get_path(pid, CHECK_PERSONALITY, 2);
        fail_if(NULL == path, "Failed to get string: %s", strerror(errno));
        fail_unless(0 == strncmp(path, "arnold_layne", 13), "Expected 'arnold_layne' got '%s'", path);

        free(path);
        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_trace_get_path_fourth) {
    PRINT_TEST_HEADER();
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        linkat(AT_FDCWD, "emily", AT_FDCWD, "arnold_layne", 0600);
    }
    else { /* parent */
        int status;
        char *path;

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        path = trace_get_path(pid, CHECK_PERSONALITY, 3);
        fail_if(NULL == path, "Failed to get string: %s", strerror(errno));
        fail_unless(0 == strncmp(path, "arnold_layne", 13), "Expected 'arnold_layne' got '%s'", path);

        free(path);
        kill(pid, SIGTERM);
    }
}
END_TEST

Suite *trace_suite_create(void) {
    Suite *s = suite_create("trace");

    /* ptrace_* test cases */
    TCase *tc_trace = tcase_create("trace");
    tcase_add_checked_fixture(tc_trace, NULL, trace_teardown);
    tcase_add_test(tc_trace, check_trace_event_e_stop);
    tcase_add_test(tc_trace, check_trace_event_e_syscall);
    tcase_add_test(tc_trace, check_trace_event_e_genuine);
    tcase_add_test(tc_trace, check_trace_event_e_exit);
    tcase_add_test(tc_trace, check_trace_event_e_exit_signal);
    tcase_add_test(tc_trace, check_trace_get_syscall);
    tcase_add_test(tc_trace, check_trace_set_syscall);
    tcase_add_test(tc_trace, check_trace_get_path_first);
    tcase_add_test(tc_trace, check_trace_get_path_second);
    tcase_add_test(tc_trace, check_trace_get_path_third);
    tcase_add_test(tc_trace, check_trace_get_path_fourth);
    suite_add_tcase(s, tc_trace);

    return s;
}
