/* Sydbox testcases for children.c
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <check.h>

#include "../src/defs.h"
#include "check_sydbox.h"

START_TEST(check_tchild_new) {
    struct tchild *tc = NULL;

    tchild_new(&tc, 666);

    fail_unless(NULL != tc);
    fail_unless(666 == tc->pid);
    fail_unless(tc->flags & TCHILD_NEEDSETUP);
    fail_if(tc->flags & TCHILD_INSYSCALL);
    fail_unless(0xbadca11 == tc->syscall);
    fail_unless(-1 == tc->retval);

    tchild_free(&tc);
}
END_TEST

START_TEST(check_tchild_free) {
    struct tchild *tc = NULL;

    tchild_new(&tc, 666);
    tchild_free(&tc);

    fail_unless(NULL == tc);
}
END_TEST

START_TEST(check_tchild_delete_first) {
    struct tchild *tc = NULL;

    tchild_new(&tc, 666);
    tchild_delete(&tc, 666);

    fail_unless(NULL == tc);
}
END_TEST

START_TEST(check_tchild_delete) {
    int i = 0;
    struct tchild *tc = NULL;
    struct tchild *curtc = NULL;

    tchild_new(&tc, 666);
    tchild_new(&tc, 667);
    tchild_new(&tc, 668);

    tchild_delete(&tc, 666);

    curtc = tc;
    while (NULL != curtc) {
        fail_unless(666 != curtc->pid, "Deleted pid found at node %d", i++);
        curtc = curtc->next;
    }

    tchild_free(&tc);
}
END_TEST

START_TEST(check_tchild_find) {
    struct tchild *tc = NULL;

    for(int i = 666; i < 670; i++)
        tchild_new(&tc, i);
    for(int i = 666; i < 670; i++)
        fail_unless(NULL != tchild_find(&tc, i), "Failed to find pid %d", i);
    for(int i = 670; i < 680; i++)
        fail_unless(NULL == tchild_find(&tc, i), "Found pid %d", i);
    tchild_free(&tc);
}
END_TEST

START_TEST(check_tchild_event_e_setup) {
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
        struct tchild *tc = NULL;

        tchild_new(&tc, pid);
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        ret = tchild_event(tc, status);
        fail_unless(E_SETUP == ret, "Expected E_SETUP got %d", ret);

        tchild_free(&tc);
        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_tchild_event_e_setup_premature) {
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
        struct tchild *tc = NULL;

        /* tchild_new(&tc, pid); */
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        ret = tchild_event(tc, status);
        fail_unless(E_SETUP_PREMATURE == ret, "Expected E_SETUP_PREMATURE got %d", ret);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_tchild_event_e_syscall) {
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
        struct tchild *tc = NULL;

        tchild_new(&tc, pid);
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        /* Check the event */
        ret = tchild_event(tc, status);
        fail_unless(E_SYSCALL == ret, "Expected E_SYSCALL got %d", ret);

        kill(pid, SIGTERM);
    }
}
END_TEST

/* TODO
 * START_TEST(check_tchild_event_e_fork)
 * START_TEST(check_tchild_event_e_fork_vfork)
 * START_TEST(check_tchild_event_e_fork_clone)
 * START_TEST(check_tchild_event_e_execv)
 */

START_TEST(check_tchild_event_e_genuine) {
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
        struct tchild *tc = NULL;

        tchild_new(&tc, pid);
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will receive a SIGINT */
        fail_if(0 > trace_cont(pid), "trace_cont() failed: %s", strerror(errno));
        wait(&status);

        /* Check the event */
        ret = tchild_event(tc, status);
        fail_unless(E_GENUINE == ret, "Expected E_GENUINE got %d", ret);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_tchild_event_e_exit) {
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
        struct tchild *tc = NULL;

        tchild_new(&tc, pid);
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will exit. */
        fail_if(0 > trace_cont(pid), "trace_cont() failed: %s", strerror(errno));
        wait(&status);

        /* Check the event */
        ret = tchild_event(tc, status);
        fail_unless(E_EXIT == ret, "Expected E_EXIT got %d", ret);
    }
}
END_TEST

START_TEST(check_tchild_event_e_exit_signal) {
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
        struct tchild *tc = NULL;

        tchild_new(&tc, pid);
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child. */
        fail_if(0 > trace_cont(pid), "trace_cont() failed: %s", strerror(errno));
        /* Kill it with a signal. */
        kill(pid, SIGKILL);
        wait(&status);

        /* Check the event */
        ret = tchild_event(tc, status);
        fail_unless(E_EXIT_SIGNAL == ret, "Expected E_EXIT_SIGNAL got %d", ret);
    }
}
END_TEST

Suite *children_suite_create(void) {
    Suite *s = suite_create("children");

    /* tchild_* test cases */
    TCase *tc_tchild = tcase_create("tchild");
    tcase_add_test(tc_tchild, check_tchild_new);
    tcase_add_test(tc_tchild, check_tchild_free);
    tcase_add_test(tc_tchild, check_tchild_delete_first);
    tcase_add_test(tc_tchild, check_tchild_delete);
    tcase_add_test(tc_tchild, check_tchild_find);
    tcase_add_test(tc_tchild, check_tchild_event_e_setup);
    tcase_add_test(tc_tchild, check_tchild_event_e_setup_premature);
    tcase_add_test(tc_tchild, check_tchild_event_e_syscall);
    tcase_add_test(tc_tchild, check_tchild_event_e_genuine);
    tcase_add_test(tc_tchild, check_tchild_event_e_exit);
    tcase_add_test(tc_tchild, check_tchild_event_e_exit_signal);
    suite_add_tcase(s, tc_tchild);

    return s;
}
