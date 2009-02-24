/* Sydbox testcases for util.c
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <limits.h>
#include <string.h>

#include <check.h>

#include "../src/defs.h"

START_TEST(check_util_remove_slash_begin) {
    int nslashes;
    char dest[PATH_MAX];

    nslashes = remove_slash("////dev/null", dest);
    fail_unless(3 == nslashes, "Returned wrong number of removed slashes: %d", nslashes);
    fail_unless(0 == strncmp(dest, "/dev/null", 10), "/dev/null != '%s'", dest);
}
END_TEST

START_TEST(check_util_remove_slash_middle) {
    int nslashes;
    char dest[PATH_MAX];

    nslashes = remove_slash("/dev////null", dest);
    fail_unless(3 == nslashes, "Returned wrong number of removed slashes: %d", nslashes);
    fail_unless(0 == strncmp(dest, "/dev/null", 10), "/dev/null != '%s'", dest);
}
END_TEST

START_TEST(check_util_remove_slash_end) {
    int nslashes;
    char dest[PATH_MAX];

    nslashes = remove_slash("/dev/null////", dest);
    fprintf(stderr, "'%s'\n", dest);
    fail_unless(4 == nslashes, "Returned wrong number of removed slashes: %d", nslashes);
    fail_unless(0 == strncmp(dest, "/dev/null", 10), "/dev/null != '%s'", dest);
}
END_TEST

Suite *util_suite_create(void) {
    Suite *s = suite_create("util");

    TCase *tc_util = tcase_create("util");
    tcase_add_test(tc_util, check_util_remove_slash_begin);
    tcase_add_test(tc_util, check_util_remove_slash_middle);
    tcase_add_test(tc_util, check_util_remove_slash_end);
    suite_add_tcase(s, tc_util);

    return s;
}
