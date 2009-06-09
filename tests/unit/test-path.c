/* vim: set et ts=4 sts=4 sw=4 fdm=syntax */

/*
 * Copyright (c) 2009 Saleem Abdulrasool <compnerd@compnerd.org>
 *
 * This file is part of the sydbox sandbox tool. sydbox is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * sydbox is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <glib.h>

#include <path.h>
#include <string.h>
#include <sydbox-config.h>

static void
test1 (void)
{
    GSList *pathlist = NULL;

    pathnode_new (&pathlist, "/dev/null", 1);
    g_assert_cmpstr (pathlist->data, ==, "/dev/null");
    g_assert (pathlist->next == NULL);

    pathnode_free (&pathlist);
}

static void
test2 (void)
{
    GSList *pathlist = NULL;
    gchar *old_home;

    old_home = g_strdup (g_getenv ("HOME"));
    if (g_setenv ("HOME", "/home/sydbox", TRUE)) {
        pathnode_new (&pathlist, "${HOME}/.sydbox", 1);
        g_assert_cmpstr (pathlist->data, ==, "/home/sydbox/.sydbox");
    }
    g_setenv ("HOME", old_home, TRUE);
    g_free (old_home);
}

static void
test3 (void)
{
    GSList *pathlist = NULL;

    pathnode_new (&pathlist, "$(echo -n /home/sydbox)/.sydbox", 1);
    g_assert_cmpstr (pathlist->data, ==, "/home/sydbox/.sydbox");
}

static void
test4 (void)
{
    GSList *pathlist = NULL;

    pathnode_new (&pathlist, "/dev/null", 1);
    pathnode_free (&pathlist);
    g_assert (pathlist == NULL);
}

static void
test5 (void)
{
    GSList *pathlist = NULL, *entry;
    const gchar env[] = "foo:bar:baz";
    gboolean seen_foo = FALSE, seen_bar = FALSE, seen_baz = FALSE;
    gint retval;

    retval = pathlist_init (&pathlist, env);
    g_assert_cmpint (retval, ==, 3);

    for (entry = pathlist; entry; entry = g_slist_next (entry))
        if (strcmp (entry->data, "foo") == 0)
            seen_foo = TRUE;
        else if (strcmp (entry->data, "bar") == 0)
            seen_bar = TRUE;
        else if (strcmp (entry->data, "baz") == 0)
            seen_baz = TRUE;
        else
            g_assert_not_reached ();

    pathnode_free (&pathlist);

    if (! seen_foo) {
        g_printerr ("did not find 'foo' in path list");
        g_assert (seen_foo);
    }

    if (! seen_bar) {
        g_printerr ("did not find 'bar' in path list");
        g_assert (seen_bar);
    }

    if (! seen_baz) {
        g_printerr ("did not find 'baz' in path list");
        g_assert (seen_baz);
    }
}

static void
test6 (void)
{
    g_assert (pathlist_init (NULL, NULL) == NULL);
}

static void
test7 (void)
{
    GSList *pathlist = NULL;
    const gchar env[] = "foo::bar::baz::::::";
    gint retval;

    retval = pathlist_init (&pathlist, env);
    g_assert_cmpint (retval, ==, 3);
    pathnode_free (&pathlist);
}

static void
test8 (void)
{
    GSList *pathlist = NULL;

    pathnode_new (&pathlist, "/dev/null", 1);
    pathnode_delete (&pathlist, "/dev/null");

    g_assert (pathlist == NULL);
}

static void
test9 (void)
{
    GSList *pathlist = NULL, *entry;

    pathnode_new (&pathlist, "/dev/null", 1);
    pathnode_new (&pathlist, "/dev/zero", 1);
    pathnode_new (&pathlist, "/dev/random", 1);

    pathnode_delete (&pathlist, "/dev/null");

    for (entry = pathlist; entry; entry = g_slist_next (entry))
        g_assert_cmpstr (entry->data, !=, "/dev/null");

    pathnode_free (&pathlist);
}

static void
test10 (void)
{
    GSList *pathlist = NULL;
    const gchar env[] = "/dev";

    pathlist_init (&pathlist, env);
    g_assert_cmpint (pathlist_check (pathlist, "/dev/zero"), !=, 0);
    g_assert_cmpint (pathlist_check (pathlist, "/dev/input/mice"), !=, 0);
    g_assert_cmpint (pathlist_check (pathlist, "/dev/mapper/control"), !=, 0);

    g_assert_cmpint (pathlist_check (pathlist, "/"), ==, 0);
    g_assert_cmpint (pathlist_check (pathlist, "/d"), ==, 0);
    g_assert_cmpint (pathlist_check (pathlist, "/de"), ==, 0);
    g_assert_cmpint (pathlist_check (pathlist, "/foo"), ==, 0);
    g_assert_cmpint (pathlist_check (pathlist, "/devzero"), ==, 0);
    g_assert_cmpint (pathlist_check (pathlist, "/foo/dev"), ==, 0);

    pathnode_free (&pathlist);
}

static void
test11 (void)
{
    GSList *pathlist = NULL;
    const gchar env[] = "/";

    pathlist_init (&pathlist, env);
    g_assert_cmpint (pathlist_check (pathlist, "/dev"), !=, 0);
    pathnode_free (&pathlist);
}

static void
no_log (const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
}

int
main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_log_set_default_handler (no_log, NULL);

    g_test_add_func ("/path/path-node/new", test1);
    g_test_add_func ("/path/path-node/new/expand-env", test2);
    g_test_add_func ("/path/path-node/new/expand-subshell", test3);
    g_test_add_func ("/path/path-node/free", test4);

    g_test_add_func ("/path/path-list/init", test5);
    g_test_add_func ("/path/path-list/init/unset", test6);
    g_test_add_func ("/path/path-list/init/ignore-empty", test7);

    g_test_add_func ("/path/path-list/delete-first", test8);
    g_test_add_func ("/path/path-list/delete", test9);

    g_test_add_func ("/path/path-list/check/path", test10);
    g_test_add_func ("/path/path-list/check/root", test11);

    return g_test_run ();
}

