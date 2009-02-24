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

#include <check.h>

#include "../src/defs.h"
#include "check_sydbox.h"

START_TEST(check_pathnode_new) {
    struct pathnode *head = NULL;

    pathnode_new(&head, "/dev/null");
    fail_unless(0 == strncmp(head->pathname, "/dev/null", 10), "Path not set correctly on creation");
    fail_unless(NULL == head->next, "Next node not set correctly on creation");
}
END_TEST

START_TEST(check_pathnode_free) {
    struct pathnode *head = NULL;

    pathnode_new(&head, "/dev/null");
    pathnode_free(&head);
    fail_unless(NULL == head, "head node not NULL after free()");
}
END_TEST

START_TEST(check_pathlist_init_unset) {
    fail_unless(0 == pathlist_init(NULL, NULL), "Didn't return 0 when environment variable isn't set");
}
END_TEST

START_TEST(check_pathlist_init) {
    const char env[] = "foo:bar:baz";
    int seen_foo = 0, seen_bar = 0, seen_baz = 0;
    struct pathnode *plist = NULL;
    struct pathnode *curnode = NULL;

    int ret = pathlist_init(&plist, env);
    fail_unless(3 == ret, "Number of paths not correct, expected: 3 got: %d", ret);
    curnode = plist;
    while (NULL != curnode) {
        if (0 == strncmp("foo", curnode->pathname, 4))
            seen_foo = 1;
        else if (0 == strncmp("bar", curnode->pathname, 4))
            seen_bar = 1;
        else if (0 == strncmp("baz", curnode->pathname, 4))
            seen_baz = 1;
        else
            fail("Unknown pathname in pathlist: '%s'", curnode->pathname);
        curnode = curnode->next;
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
    const char env[] = "foo::bar::baz::::::";
    struct pathnode *plist = NULL;

    fail_unless(3 == pathlist_init(&plist, env), "Didn't ignore empty pathnames in environment variable.");
    pathnode_free(&plist);
}
END_TEST

START_TEST(check_pathlist_check) {
    const char env[] = "/dev";
    struct pathnode *plist = NULL;

    pathlist_init(&plist, env);

    fail_unless(0 != pathlist_check(&plist, "/dev/zero"),
            "Failed for /dev/zero when /dev was an allowed pathname.");
    fail_unless(0 != pathlist_check(&plist, "/dev/mapper/control"),
            "Failed for /dev/mapper/control when /dev was an allowed pathname.");
    fail_unless(0 != pathlist_check(&plist, "/dev/input/mice"),
            "Failed for /dev/input/mice when /dev was an allowed pathname");

    fail_unless(0 == pathlist_check(&plist, "/"),
            "Succeeded for / when /dev was the only allowed pathname.");
    fail_unless(0 == pathlist_check(&plist, "/d"),
            "Succeeded for /d when /dev was the only allowed pathname.");
    fail_unless(0 == pathlist_check(&plist, "/de"),
            "Succeeded for /de when /dev was the only allowed pathname.");
    fail_unless(0 == pathlist_check(&plist, "/devzero"),
            "Succeded for /devzero when /dev was the only allowed pathname.");
    fail_unless(0 == pathlist_check(&plist, "/foo"),
            "Succeeded for /foo when /dev was the only allowed pathname.");
    fail_unless(0 == pathlist_check(&plist, "/foo/dev"),
            "Succeeded for /foo/dev when /dev was the only allowed pathname.");
}
END_TEST

Suite *path_suite_create(void) {
    Suite *s = suite_create("path");

    /* pathnode_* test cases */
    TCase *tc_pathnode = tcase_create("pathnode");
    tcase_add_test(tc_pathnode, check_pathnode_new);
    tcase_add_test(tc_pathnode, check_pathnode_free);
    suite_add_tcase(s, tc_pathnode);

    /* pathlist_* test cases */
    TCase *tc_pathlist = tcase_create("pathlist");
    tcase_add_test(tc_pathlist, check_pathlist_init_unset);
    tcase_add_test(tc_pathlist, check_pathlist_init);
    tcase_add_test(tc_pathlist, check_pathlist_init_ignore_empty);
    tcase_add_test(tc_pathlist, check_pathlist_check);
    suite_add_tcase(s, tc_pathlist);

    return s;
}
