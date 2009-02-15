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
    fail_unless(0 != tc->need_setup);
    fail_unless(0 == tc->in_syscall);
    fail_unless(0xbadca11 == tc->orig_syscall);
    fail_unless(-1 == tc->error_code);

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
        fail_unless(666 != curtc->pid,
                "Deleted pid found at node %d", i++);
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
        fail_unless(NULL != tchild_find(&tc, i),
                "Failed to find pid %d", i);
    for(int i = 670; i < 680; i++)
        fail_unless(NULL == tchild_find(&tc, i),
                "Found pid %d", i);
    tchild_free(&tc);
}
END_TEST

/* FIXME why doesn't this work? */
#if 0
START_TEST(check_tchild_setup) {
    pid_t pid;
    struct tchild *tc = NULL;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        for(;;)
            sleep(1);
    }
    else { /* parent */
        tchild_new(&tc, pid);
        tchild_setup(tc);
        fail_unless(0 == tc->need_setup);

        kill(pid, SIGTERM);
    }
}
END_TEST
#endif

Suite *children_suite_create(void) {
    Suite *s = suite_create("children");

    /* tchild_* test cases */
    TCase *tc_tchild = tcase_create("tchild");
    tcase_add_test(tc_tchild, check_tchild_new);
    tcase_add_test(tc_tchild, check_tchild_free);
    tcase_add_test(tc_tchild, check_tchild_delete_first);
    tcase_add_test(tc_tchild, check_tchild_delete);
    tcase_add_test(tc_tchild, check_tchild_find);
    suite_add_tcase(s, tc_tchild);

    return s;
}
