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
#include <sydbox-utils.h>

static void
test1 (void)
{
    gchar *path = sydbox_compress_path ("////dev/null");
    g_assert_cmpstr (path, ==, "/dev/null");
    g_free (path);
}

static void
test2 (void)
{
    gchar *path = sydbox_compress_path ("/dev////null");
    g_assert_cmpstr (path, ==, "/dev/null");
    g_free (path);
}

static void
test3 (void)
{
    gchar *path = sydbox_compress_path ("/dev/null////");
    g_assert_cmpstr (path, ==, "/dev/null");
    g_free (path);
}

static void
test4 (void)
{
    gchar *path = sydbox_compress_path ("/");
    g_assert_cmpstr (path, ==, "/");
    g_free (path);
}

static void
test5 (void)
{
    gchar *path = sydbox_compress_path ("////");
    g_assert_cmpstr (path, ==, "/");
    g_free (path);
}

static void
test6 (void)
{
    gchar *path = sydbox_compress_path ("");
    g_assert_cmpstr (path, ==, "");
    g_free (path);
}

int
main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/utils/compress-path/compress-beginning", test1);
    g_test_add_func ("/utils/compress-path/compress-middle", test2);
    g_test_add_func ("/utils/compress-path/compress-end", test3);

    g_test_add_func ("/utils/compress-path/single-slash", test4);
    g_test_add_func ("/utils/compress-path/only-slashes", test5);
    g_test_add_func ("/utils/compress-path/empty-string", test6);

    return g_test_run ();
}

