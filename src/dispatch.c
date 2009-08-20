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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <stdbool.h>
#include <asm/unistd.h>

#include <glib.h> // G_GNUC_UNUSED

#include "dispatch.h"
#include "dispatch-table.h"

static const struct syscall_name {
    int no;
    const char *name;
} sysnames[] = {
#include "syscallent.h"
    {-1,    NULL}
};

int dispatch_flags(int personality G_GNUC_UNUSED, int sno)
{
    for (unsigned int i = 0; -1 != syscalls[i].no; i++) {
        if (syscalls[i].no == sno)
            return syscalls[i].flags;
    }
    return -1;
}

const char *dispatch_name(int personality G_GNUC_UNUSED, int sno)
{
    for (unsigned int i = 0; NULL != sysnames[i].name; i++) {
        if (sysnames[i].no == sno)
            return sysnames[i].name;
    }
    return UNKNOWN_SYSCALL;
}

inline const char *dispatch_mode(int personality G_GNUC_UNUSED)
{
    const char *mode;
#if defined(I386)
    mode = "32 bit";
#elif defined(IA64)
    mode = "64 bit";
#elif defined(POWERPC)
    mode = "64 bit";
#else
#error unsupported architecture
#endif
    return mode;
}

bool dispatch_chdir(int personality G_GNUC_UNUSED, int sno)
{
    return IS_CHDIR(sno);
}

#if defined(POWERPC) || defined(SPARC64)
bool dispatch_clone(int personality G_GNUC_UNUSED, int sno)
{
    return IS_CLONE(sno);
}
#endif // defined(POWERPC)

