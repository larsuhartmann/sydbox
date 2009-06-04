/* vim: set et ts=4 sts=4 sw=4 fdm=syntax : */

/*
 * sydbox test cases for sydbox-util.c
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <glib.h>

#include <check.h>

#include <string.h>

#include "check_sydbox.h"
#include "../src/sydbox-utils.h"

START_TEST (check_sydbox_utils_sydbox_compress_path_begin) {
   PRINT_TEST_HEADER();
   gchar *dest = sydbox_compress_path ("////dev/null");
   fail_unless (strncmp (dest, "/dev/null", 10) == 0, "/dev/null != '%s'", dest);
   g_free (dest);
} END_TEST

START_TEST (check_sydbox_utils_sydbox_compress_path_middle) {
   PRINT_TEST_HEADER();
   gchar *dest = sydbox_compress_path ("/dev////null");
   fail_unless (strncmp (dest, "/dev/null", 10) == 0, "/dev/null != '%s'", dest);
   g_free (dest);
} END_TEST

START_TEST (check_sydbox_utils_sydbox_compress_path_end) {
   PRINT_TEST_HEADER();
   gchar *dest = sydbox_compress_path ("/dev/null////");
   fail_unless (strncmp (dest, "/dev/null", 10) == 0, "/dev/null != '%s'", dest);
   g_free (dest);
} END_TEST

START_TEST (check_sydbox_utils_sydbox_compress_path_single_slash) {
   PRINT_TEST_HEADER();
   gchar *dest = sydbox_compress_path ("/");
   fail_unless (strncmp (dest, "/", 2) == 0, "/ != '%s'", dest);
   g_free (dest);
} END_TEST

START_TEST (check_sydbox_utils_sydbox_compress_path_only_slashes) {
   PRINT_TEST_HEADER();
   gchar *dest = sydbox_compress_path ("////");
   fail_unless (strncmp (dest, "/", 2) == 0, "/ != '%s'", dest);
   g_free (dest);
} END_TEST

START_TEST (check_sydbox_utils_sydbox_compress_path_empty_string) {
   PRINT_TEST_HEADER();
   gchar *dest = sydbox_compress_path ("");
   fail_unless (strncmp (dest, "", 1) == 0, "'' != '%s'", dest);
   g_free (dest);
} END_TEST

Suite *
sydbox_utils_suite_create (void)
{
   Suite *s = suite_create ("sydbox-utils");

   TCase *tc_util = tcase_create ("sydbox-utils");
   tcase_add_test (tc_util, check_sydbox_utils_sydbox_compress_path_begin);
   tcase_add_test (tc_util, check_sydbox_utils_sydbox_compress_path_middle);
   tcase_add_test (tc_util, check_sydbox_utils_sydbox_compress_path_end);
   tcase_add_test (tc_util, check_sydbox_utils_sydbox_compress_path_single_slash);
   tcase_add_test (tc_util, check_sydbox_utils_sydbox_compress_path_only_slashes);
   tcase_add_test (tc_util, check_sydbox_utils_sydbox_compress_path_empty_string);

   suite_add_tcase (s, tc_util);

   return s;
}

