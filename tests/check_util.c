/* Sydbox testcases for util.c
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <check.h>

#include "../src/defs.h"
#include "../src/util.h"
#include "check_sydbox.h"

START_TEST(check_util_remove_slash_begin) {
    PRINT_TEST_HEADER;
    char *dest = remove_slash("////dev/null");
    fail_unless(0 == strncmp(dest, "/dev/null", 10), "/dev/null != '%s'", dest);
    free(dest);
}
END_TEST

START_TEST(check_util_remove_slash_middle) {
    PRINT_TEST_HEADER;
    char *dest = remove_slash("/dev////null");
    fail_unless(0 == strncmp(dest, "/dev/null", 10), "/dev/null != '%s'", dest);
    free(dest);
}
END_TEST

START_TEST(check_util_remove_slash_end) {
    PRINT_TEST_HEADER;
    char *dest = remove_slash("/dev/null////");
    fail_unless(0 == strncmp(dest, "/dev/null", 10), "/dev/null != '%s'", dest);
    free(dest);
}
END_TEST

START_TEST(check_util_remove_slash_only_slash) {
    PRINT_TEST_HEADER;
    char *dest = remove_slash("/");
    fail_unless(0 == strncmp(dest, "/", 2), "/ != '%s'", dest);
    free(dest);
}
END_TEST

START_TEST(check_util_remove_slash_only_slashes) {
    PRINT_TEST_HEADER;
    char *dest = remove_slash("////");
    fail_unless(0 == strncmp(dest, "/", 2), "/ != '%s'", dest);
    free(dest);
}
END_TEST

START_TEST(check_util_remove_slash_empty) {
    PRINT_TEST_HEADER;
    char *dest = remove_slash("");
    fail_unless(0 == strncmp(dest, "", 1));
    free(dest);
}
END_TEST

Suite *util_suite_create(void) {
    Suite *s = suite_create("util");

    TCase *tc_util = tcase_create("util");
    tcase_add_test(tc_util, check_util_remove_slash_begin);
    tcase_add_test(tc_util, check_util_remove_slash_middle);
    tcase_add_test(tc_util, check_util_remove_slash_end);
    tcase_add_test(tc_util, check_util_remove_slash_only_slash);
    tcase_add_test(tc_util, check_util_remove_slash_only_slashes);
    tcase_add_test(tc_util, check_util_remove_slash_empty);
    suite_add_tcase(s, tc_util);

    return s;
}
