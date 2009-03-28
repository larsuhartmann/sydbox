/* Sydbox testcases for path.c
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <limits.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <check.h>

#include "../src/defs.h"
#include "../src/path.h"
#include "check_sydbox.h"

START_TEST(check_pathnode_new) {
    PRINT_TEST_HEADER;
    GSList *head = NULL;

    pathnode_new(&head, "/dev/null", 1);
    fail_unless(0 == strncmp(head->data, "/dev/null", 10), "Path not set correctly on creation");
    fail_unless(NULL == head->next, "Next node not set correctly on creation");
}
END_TEST

START_TEST(check_pathnode_shell_env_expand) {
    PRINT_TEST_HEADER;

    GSList *head = NULL;
    gchar *old_home;

    old_home = g_strdup (g_getenv ("HOME"));
    if (g_setenv ("HOME", "/home/sydbox", TRUE)) {
        pathnode_new (&head, "${HOME}/.sydbox", 1);
        fail_unless (0 == strcmp (head->data, "/home/sydbox/.sydbox"), "path expansion failed");
    }
    g_setenv ("HOME", old_home, TRUE);
    g_free (old_home);
}
END_TEST

START_TEST(check_pathnode_shell_subshell_expand) {
    PRINT_TEST_HEADER;

    GSList *head = NULL;

    pathnode_new (&head, "$(echo -n /home/sydbox)/.sydbox", 1);
    fail_unless (0 == strcmp (head->data, "/home/sydbox/.sydbox"), "path expansion failed");
}
END_TEST

START_TEST(check_pathnode_free) {
    PRINT_TEST_HEADER;
    GSList *head = NULL;

    pathnode_new(&head, "/dev/null", 1);
    pathnode_free(&head);
    fail_unless(NULL == head, "head node not NULL after pathnode_free()");
}
END_TEST

START_TEST(check_pathnode_delete_first) {
    PRINT_TEST_HEADER;
    GSList *head = NULL;

    pathnode_new(&head, "/dev/null", 1);
    pathnode_delete(&head, "/dev/null");

    fail_unless(NULL == head);
}
END_TEST

START_TEST(check_pathnode_delete) {
    PRINT_TEST_HEADER;
    int i = 0;
    GSList *plist = NULL;
    GSList *walk = NULL;

    pathnode_new(&plist, "/dev/null", 1);
    pathnode_new(&plist, "/dev/zero", 1);
    pathnode_new(&plist, "/dev/random", 1);

    pathnode_delete(&plist, "/dev/null");

    walk = plist;
    while (NULL != walk) {
        fail_if(0 == strncmp(walk->data, "/dev/null", 10), "Deleted path found at node %d", i++);
        walk = g_slist_next(walk);
    }

    pathnode_free(&plist);
}
END_TEST

START_TEST(check_pathlist_init_unset) {
    PRINT_TEST_HEADER;
    fail_unless(0 == pathlist_init(NULL, NULL), "Didn't return 0 when environment variable isn't set");
}
END_TEST

START_TEST(check_pathlist_init) {
    PRINT_TEST_HEADER;
    const char env[] = "foo:bar:baz";
    int seen_foo = 0, seen_bar = 0, seen_baz = 0;
    GSList *plist = NULL;
    GSList *walk = NULL;

    int ret = pathlist_init(&plist, env);
    fail_unless(3 == ret, "Number of paths not correct, expected: 3 got: %d", ret);
    walk = plist;
    while (NULL != walk) {
        if (0 == strncmp(walk->data, "foo", 4))
            seen_foo = 1;
        else if (0 == strncmp(walk->data, "bar", 4))
            seen_bar = 1;
        else if (0 == strncmp(walk->data, "baz", 4))
            seen_baz = 1;
        else
            fail("Unknown path in pathlist: `%s'", walk->data);
        walk = g_slist_next(walk);
    }
    pathnode_free(&plist);

    if (!seen_foo)
        fail("First element not in pathlist");
    if (!seen_bar)
        fail("Second element not in pathlist");
    if (!seen_baz)
        fail("Last element not in pathlist");
}
END_TEST

START_TEST(check_pathlist_init_ignore_empty) {
    PRINT_TEST_HEADER;
    const char env[] = "foo::bar::baz::::::";
    GSList *plist = NULL;

    fail_unless(3 == pathlist_init(&plist, env), "Didn't ignore empty paths in environment variable.");
    pathnode_free(&plist);
}
END_TEST

START_TEST(check_pathlist_check) {
    PRINT_TEST_HEADER;
    const char env[] = "/dev";
    GSList *plist = NULL;

    pathlist_init(&plist, env);

    fail_unless(0 != pathlist_check(plist, "/dev/zero"),
            "Failed for /dev/zero when /dev was an allowed path.");
    fail_unless(0 != pathlist_check(plist, "/dev/mapper/control"),
            "Failed for /dev/mapper/control when /dev was an allowed path.");
    fail_unless(0 != pathlist_check(plist, "/dev/input/mice"),
            "Failed for /dev/input/mice when /dev was an allowed path");

    fail_unless(0 == pathlist_check(plist, "/"),
            "Succeeded for / when /dev was the only allowed path.");
    fail_unless(0 == pathlist_check(plist, "/d"),
            "Succeeded for /d when /dev was the only allowed path.");
    fail_unless(0 == pathlist_check(plist, "/de"),
            "Succeeded for /de when /dev was the only allowed path.");
    fail_unless(0 == pathlist_check(plist, "/devzero"),
            "Succeded for /devzero when /dev was the only allowed path.");
    fail_unless(0 == pathlist_check(plist, "/foo"),
            "Succeeded for /foo when /dev was the only allowed path.");
    fail_unless(0 == pathlist_check(plist, "/foo/dev"),
            "Succeeded for /foo/dev when /dev was the only allowed path.");
}
END_TEST

START_TEST(check_pathlist_check_slash_only) {
    PRINT_TEST_HEADER;
    const char env[] = "/";
    GSList *plist = NULL;

    pathlist_init(&plist, env);

    fail_unless(0 != pathlist_check(plist, "/dev"),
            "Failed for /dev when / was an allowed path");
}
END_TEST

Suite *path_suite_create(void) {
    Suite *s = suite_create("path");

    /* pathnode_* test cases */
    TCase *tc_pathnode = tcase_create("pathnode");
    tcase_add_test(tc_pathnode, check_pathnode_new);
    tcase_add_test(tc_pathnode, check_pathnode_free);
    tcase_add_test(tc_pathnode, check_pathnode_shell_env_expand);
    tcase_add_test(tc_pathnode, check_pathnode_shell_subshell_expand);
    suite_add_tcase(s, tc_pathnode);

    /* pathlist_* test cases */
    TCase *tc_pathlist = tcase_create("pathlist");
    tcase_add_test(tc_pathlist, check_pathlist_init_unset);
    tcase_add_test(tc_pathlist, check_pathlist_init);
    tcase_add_test(tc_pathlist, check_pathlist_init_ignore_empty);
    tcase_add_test(tc_pathlist, check_pathlist_check);
    tcase_add_test(tc_pathlist, check_pathlist_check_slash_only);
    suite_add_tcase(s, tc_pathlist);

    return s;
}
