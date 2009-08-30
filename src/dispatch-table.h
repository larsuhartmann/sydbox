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

#ifndef SYDBOX_GUARD_DISPATCH_TABLE_H
#define SYDBOX_GUARD_DISPATCH_TABLE_H 1

#include "flags.h"

// System call dispatch table
static const struct syscall_def {
    int no;
    int flags;
} syscalls[] = {
    {__NR_chmod,        CHECK_PATH},
    {__NR_chown,        CHECK_PATH},
#if defined(__NR_chown32)
    {__NR_chown32,      CHECK_PATH},
#endif
    {__NR_open,         CHECK_PATH | OPEN_MODE},
    {__NR_creat,        CHECK_PATH | CAN_CREAT},
    {__NR_stat,         MAGIC_STAT},
#if defined(__NR_stat64)
    {__NR_stat64,       MAGIC_STAT},
#endif
    {__NR_lstat,        MAGIC_STAT},
#if defined(__NR_lstat64)
    {__NR_lstat64,      MAGIC_STAT},
#endif
    {__NR_lchown,       CHECK_PATH | DONT_RESOLV},
#if defined(__NR_lchown32)
    {__NR_lchown32,     CHECK_PATH | DONT_RESOLV},
#endif
    {__NR_link,         CHECK_PATH | CHECK_PATH2 | MUST_CREAT2 | DONT_RESOLV},
    {__NR_mkdir,        CHECK_PATH | MUST_CREAT},
    {__NR_mknod,        CHECK_PATH | MUST_CREAT},
    {__NR_access,       CHECK_PATH | ACCESS_MODE},
    {__NR_rename,       CHECK_PATH | CHECK_PATH2 | CAN_CREAT2 | DONT_RESOLV},
    {__NR_rmdir,        CHECK_PATH},
    {__NR_symlink,      CHECK_PATH2 | MUST_CREAT2 | DONT_RESOLV},
    {__NR_truncate,     CHECK_PATH},
#if defined(__NR_truncate64)
    {__NR_truncate64,   CHECK_PATH},
#endif
    {__NR_mount,        CHECK_PATH2},
#if defined(__NR_umount)
    {__NR_umount,       CHECK_PATH},
#endif
#if defined(__NR_umount2)
    {__NR_umount2,      CHECK_PATH},
#endif
#if defined(__NR_utime)
    {__NR_utime,        CHECK_PATH},
#endif
#if defined(__NR_utimes)
    {__NR_utimes,       CHECK_PATH},
#endif
    {__NR_unlink,       CHECK_PATH | DONT_RESOLV},
    {__NR_openat,       CHECK_PATH_AT | OPEN_MODE_AT},
    {__NR_mkdirat,      CHECK_PATH_AT | MUST_CREAT_AT},
    {__NR_mknodat,      CHECK_PATH_AT | MUST_CREAT_AT},
    {__NR_fchownat,     CHECK_PATH_AT | IF_AT_SYMLINK_NOFOLLOW4},
    {__NR_unlinkat,     CHECK_PATH_AT | IF_AT_REMOVEDIR2},
    {__NR_renameat,     CHECK_PATH_AT | CHECK_PATH_AT2 | CAN_CREAT_AT2 | DONT_RESOLV},
    {__NR_linkat,       CHECK_PATH_AT | CHECK_PATH_AT2 | MUST_CREAT_AT2 | IF_AT_SYMLINK_FOLLOW4},
    {__NR_symlinkat,    CHECK_PATH_AT1 | MUST_CREAT_AT1 | DONT_RESOLV},
    {__NR_fchmodat,     CHECK_PATH_AT | IF_AT_SYMLINK_NOFOLLOW3},
    {__NR_faccessat,    CHECK_PATH_AT | ACCESS_MODE_AT},
#if defined(__NR_socketcall)
    {__NR_socketcall,   DECODE_SOCKETCALL},
#endif
#if defined(__NR_connect)
    {__NR_connect,      CONNECT_CALL},
#endif
#if defined(__NR_bind)
    {__NR_bind,         BIND_CALL},
#endif
#if defined(__NR_sendto)
    {__NR_sendto,       SENDTO_CALL},
#endif
    {__NR_execve,       EXEC_CALL},
    {-1,                -1},
};

#endif // SYDBOX_GUARD_DISPATCH_TABLE_H

