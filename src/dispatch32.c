/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <polatel@gmail.com>
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

#include <stdbool.h>
#include <stdio.h>
#include <asm/unistd_32.h>

#include <glib.h>

#include "dispatch.h"
#include "dispatch-table.h"

static const struct syscall_name {
    int no;
    const char *name;
} sysnames[] = {
#include "syscallent32.h"
    {-1,    NULL}
};

static GHashTable *flags32 = NULL;

void dispatch_init32(void)
{
    if (flags32 != NULL)
        return;

    flags32 = g_hash_table_new(g_direct_hash, g_direct_equal);
    for (unsigned int i = 0; -1 != syscalls[i].no; i++)
        g_hash_table_insert(flags32, GINT_TO_POINTER(syscalls[i].no), GINT_TO_POINTER(syscalls[i].flags));
}

void dispatch_free32(void)
{
    if (flags32 != NULL)
        g_hash_table_destroy(flags32);
    flags32 = NULL;
}

int dispatch_lookup32(int sno)
{
    gpointer f;

    g_assert(flags32 != NULL);
    f = g_hash_table_lookup(flags32, GINT_TO_POINTER(sno));
    return (f == NULL) ? -1 : GPOINTER_TO_INT(f);
}

const char *dispatch_name32(int sno)
{
    for (unsigned int i = 0; NULL != sysnames[i].name; i++) {
        if (sysnames[i].no == sno)
            return sysnames[i].name;
    }
    return UNKNOWN_SYSCALL;
}

bool dispatch_maybind32(int sno)
{
    return (__NR_socketcall == sno);
}

