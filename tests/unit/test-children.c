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
#include <children.h>
#include <sydbox-config.h>

static void
test1 (void)
{
    GSList *children = NULL;
    struct tchild *child;

    tchild_new (&children, 666);
    g_assert (children);

    child = (struct tchild *) children->data;
    g_assert (child);
    g_assert_cmpint (child->pid, ==, 666);
    g_assert (child->flags & TCHILD_NEEDSETUP);
    g_assert (!(child->flags & TCHILD_INSYSCALL));
    g_assert_cmpint (child->sno, ==, 0xbadca11);
    g_assert_cmpint (child->retval, ==, -1);

    tchild_free (&children);
}

static void
test2 (void)
{
    GSList *children = NULL;

    tchild_new (&children, 666);
    tchild_free (&children);

    g_assert (! children);
}

static void
test3 (void)
{
    GSList *children = NULL;

    tchild_new (&children, 666);
    tchild_delete (&children, 666);

    g_assert (! children);
}

static void
test4 (void)
{
    GSList *children = NULL;
    GSList *entry = NULL;

    tchild_new (&children, 666);
    tchild_new (&children, 667);
    tchild_new (&children, 668);

    tchild_delete (&children, 666);

    for (entry = children; entry; entry = g_slist_next (entry))
        g_assert_cmpint (((struct tchild *)entry->data)->pid, !=, 666);

    tchild_free (&children);
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

    g_test_add_func ("/children/new", test1);
    g_test_add_func ("/children/free", test2);

    g_test_add_func ("/children/delete-first", test3);
    g_test_add_func ("/children/delete", test4);

    return g_test_run ();
}

