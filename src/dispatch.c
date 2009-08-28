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

#include <glib.h>

#include "flags.h"
#include "dispatch.h"
#include "dispatch-table.h"

static const struct syscall_name {
    int no;
    const char *name;
} sysnames[] = {
#include "syscallent.h"
    {-1,    NULL}
};

static GHashTable *flags = NULL;

static int dispatch_flags(int personality G_GNUC_UNUSED, int sno)
{
    for (unsigned int i = 0; -1 != syscalls[i].no; i++) {
        if (syscalls[i].no == sno)
            return syscalls[i].flags;
    }
    return -1;
}

void dispatch_init(void)
{
    if (flags != NULL)
        return;

    flags = g_hash_table_new(g_direct_hash, g_direct_equal);
#define REGISTER_SYSCALL(no)                                                                            \
    do {                                                                                                \
        g_hash_table_insert(flags, GINT_TO_POINTER((no)), GINT_TO_POINTER(dispatch_flags(-1, (no))));   \
    } while (0)

    REGISTER_SYSCALL(__NR_chmod);
    REGISTER_SYSCALL(__NR_chown);
#if defined(__NR_chown32)
    REGISTER_SYSCALL(__NR_chown32);
#endif
    REGISTER_SYSCALL(__NR_open);
    REGISTER_SYSCALL(__NR_creat);
    REGISTER_SYSCALL(__NR_stat);
#if defined(__NR_stat64)
    REGISTER_SYSCALL(__NR_stat64);
#endif
    REGISTER_SYSCALL(__NR_lchown);
#if defined(__NR_lchown32)
    REGISTER_SYSCALL(__NR_lchown32);
#endif
    REGISTER_SYSCALL(__NR_link);
    REGISTER_SYSCALL(__NR_mkdir);
    REGISTER_SYSCALL(__NR_mknod);
    REGISTER_SYSCALL(__NR_access);
    REGISTER_SYSCALL(__NR_rename);
    REGISTER_SYSCALL(__NR_rmdir);
    REGISTER_SYSCALL(__NR_symlink);
    REGISTER_SYSCALL(__NR_truncate);
#if defined(__NR_truncate64)
    REGISTER_SYSCALL(__NR_truncate64);
#endif
    REGISTER_SYSCALL(__NR_mount);
#if defined(__NR_umount)
    REGISTER_SYSCALL(__NR_umount);
#endif
#if defined(__NR_umount2)
    REGISTER_SYSCALL(__NR_umount2);
#endif
#if defined(__NR_utime)
    REGISTER_SYSCALL(__NR_utime);
#endif
#if defined(__NR_utimes)
    REGISTER_SYSCALL(__NR_utimes);
#endif
    REGISTER_SYSCALL(__NR_unlink);
    REGISTER_SYSCALL(__NR_openat);
    REGISTER_SYSCALL(__NR_mkdirat);
    REGISTER_SYSCALL(__NR_mknodat);
    REGISTER_SYSCALL(__NR_fchownat);
    REGISTER_SYSCALL(__NR_unlinkat);
    REGISTER_SYSCALL(__NR_renameat);
    REGISTER_SYSCALL(__NR_linkat);
    REGISTER_SYSCALL(__NR_symlinkat);
    REGISTER_SYSCALL(__NR_fchmodat);
    REGISTER_SYSCALL(__NR_faccessat);
#if defined(__NR_socketcall)
    REGISTER_SYSCALL(__NR_socketcall);
#endif
#if defined(__NR_socket)
    REGISTER_SYSCALL(__NR_socket);
#endif
#if defined(__NR_connect)
    REGISTER_SYSCALL(__NR_connect);
#endif
#if defined(__NR_bind)
    REGISTER_SYSCALL(__NR_bind);
#endif
    REGISTER_SYSCALL(__NR_execve);
    REGISTER_SYSCALL(__NR_chdir);
    REGISTER_SYSCALL(__NR_fchdir);
#if defined(POWERPC)
    REGISTER_SYSCALL(__NR_clone);
#endif
#undef REGISTER_SYSCALL
}

void dispatch_free(void)
{
    if (flags != NULL)
        g_hash_table_destroy(flags);
    flags = NULL;
}

int dispatch_lookup(int personality G_GNUC_UNUSED, int sno)
{
    gpointer f;

    g_assert(flags != NULL);
    f = g_hash_table_lookup(flags, GINT_TO_POINTER(sno));
    return (f == NULL) ? -1 : GPOINTER_TO_INT(f);
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

bool dispatch_maybind(int personality G_GNUC_UNUSED, int sno)
{
#if defined(I386) || defined(POWERPC)
    return (__NR_socketcall == sno);
#elif defined(IA64)
    return (__NR_bind == sno);
#else
#error unsupported architecture
#endif
}

