/* Sydbox testcases for children.c
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

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
}
END_TEST

Suite *children_suite_create(void) {
    Suite *s = suite_create("children");

    /* tchild_* test cases */
    TCase *tc_tchild = tcase_create("tchild");
    tcase_add_test(tc_tchild, check_tchild_new);
    suite_add_tcase(s, tc_tchild);

    return s;
}
