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

#include <glib.h>
#include <check.h>

#include "../src/children.h"
#include "check_sydbox.h"

START_TEST(check_tchild_new) {
    PRINT_TEST_HEADER;
    GSList *tc = NULL;
    struct tchild *child;

    tchild_new(&tc, 666, -1);

    fail_unless(NULL != tc);
    child = (struct tchild *) tc->data;
    fail_unless(NULL != child);
    fail_unless(666 == child->pid);
    fail_unless(child->flags & TCHILD_NEEDSETUP);
    fail_if(child->flags & TCHILD_INSYSCALL);
    fail_unless(0xbadca11 == child->sno);
    fail_unless(-1 == child->retval);

    tchild_free(&tc);
}
END_TEST

START_TEST(check_tchild_free) {
    PRINT_TEST_HEADER;
    GSList *tc = NULL;

    tchild_new(&tc, 666, -1);
    tchild_free(&tc);

    fail_unless(NULL == tc);
}
END_TEST

START_TEST(check_tchild_delete_first) {
    PRINT_TEST_HEADER;
    GSList *tc = NULL;

    tchild_new(&tc, 666, -1);
    tchild_delete(&tc, 666);

    fail_unless(NULL == tc);
}
END_TEST

START_TEST(check_tchild_delete) {
    PRINT_TEST_HEADER;
    int i = 0;
    GSList *tc = NULL;
    GSList *walk = NULL;

    tchild_new(&tc, 666, -1);
    tchild_new(&tc, 667, 666);
    tchild_new(&tc, 668, 667);

    tchild_delete(&tc, 666);

    walk = tc;
    while (NULL != walk) {
        struct tchild *child = (struct tchild *) walk->data;
        fail_unless(666 != child->pid, "Deleted pid found at node %d", i++);
        walk = g_slist_next(walk);
    }

    tchild_free(&tc);
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
    suite_add_tcase(s, tc_tchild);

    return s;
}
