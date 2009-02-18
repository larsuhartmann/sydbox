/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel
 * Based in part upon catbox which is:
 *  Copyright (c) 2006-2007 TUBITAK/UEKAE
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

#define _ATFILE_SOURCE /* AT_FDCWD */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <sys/ptrace.h>

#include "defs.h"

/* System call dispatch flags */
#define RETURNS_FD      (1 << 0) /* The function returns a file descriptor */
#define OPEN_MODE       (1 << 1) /* Check the mode argument of open() */
#define OPEN_MODE_AT    (1 << 2) /* Check the mode argument of openat() */
#define ACCESS_MODE     (1 << 3) /* Check the mode argument of access() */
#define CHECK_PATH      (1 << 4) /* First argument should be a valid path */
#define CHECK_PATH2     (1 << 5) /* Second argument should be a valid path */
#define CHECK_PATH_AT   (1 << 6) /* CHECK_PATH for at suffixed functions. */
#define DONT_RESOLV     (1 << 7) /* Don't resolve symlinks */
#define NET_CALL        (1 << 8) /* Allowing the system call depends on the net flag */

/* System call dispatch table */
static struct syscall_def {
    int no;
    const char *name;
    unsigned int flags;
} system_calls[] = {
    {__NR_chmod,        "chmod",        CHECK_PATH},
    {__NR_chown,        "chown",        CHECK_PATH},
#if defined(I386)
    {__NR_chown32,      "chown32",      CHECK_PATH},
#endif
    {__NR_open,         "open",         CHECK_PATH | RETURNS_FD | OPEN_MODE},
    {__NR_creat,        "creat",        CHECK_PATH},
    {__NR_lchown,       "lchown",       CHECK_PATH | DONT_RESOLV},
#if defined(I386)
    {__NR_lchown32,     "lchown32",     CHECK_PATH | DONT_RESOLV},
#endif
    {__NR_link,         "link",         CHECK_PATH},
    {__NR_mkdir,        "mkdir",        CHECK_PATH},
    {__NR_mknod,        "mknod",        CHECK_PATH},
    {__NR_access,       "access",       CHECK_PATH | ACCESS_MODE},
    {__NR_rename,       "rename",       CHECK_PATH | CHECK_PATH2},
    {__NR_rmdir,        "rmdir",        CHECK_PATH},
    {__NR_symlink,      "symlink",      CHECK_PATH2 | DONT_RESOLV},
    {__NR_truncate,     "truncate",     CHECK_PATH},
#if defined(I386)
    {__NR_truncate64,   "truncate64",   CHECK_PATH},
#endif
    {__NR_mount,        "mount",        CHECK_PATH},
#if defined(I386)
    {__NR_umount,       "umount",       CHECK_PATH},
#endif
    {__NR_umount2,      "umount2",      CHECK_PATH},
    {__NR_utime,        "utime",        CHECK_PATH},
    {__NR_unlink,       "unlink",       CHECK_PATH},
    {__NR_openat,       "openat",       CHECK_PATH_AT | OPEN_MODE_AT | RETURNS_FD},
    {__NR_mkdirat,      "mkdirat",      CHECK_PATH_AT},
    {__NR_mknodat,      "mknodat",      CHECK_PATH_AT},
    {__NR_fchownat,     "fchownat",     CHECK_PATH_AT},
    {__NR_unlinkat,     "unlinkat",     CHECK_PATH_AT},
    {__NR_renameat,     "renameat",     0},
    {__NR_linkat,       "linkat",       CHECK_PATH_AT},
    {__NR_symlinkat,    "symlinkat",    0},
    {__NR_fchmodat,     "fchmodat",     CHECK_PATH_AT},
    {__NR_faccessat,    "faccessat",    0},
#if defined(I386)
    {__NR_socketcall,   "socketcall",   NET_CALL},
#elif defined(X86_64)
    {__NR_socket,       "socket",       NET_CALL},
#endif
    {0,                 NULL,           0}
};

int syscall_check_path(context_t *ctx, struct tchild *child,
        struct decision *decs, int arg, int sflags, const char *sname) {
    int issymlink;
    char pathname[PATH_MAX];
    char *rpath;

    assert(1 == arg || 2 == arg);

    if (sflags & CHECK_PATH_AT) {
        int dirfd;
        dirfd = ptrace(PTRACE_PEEKUSER, child->pid, PARAM1, NULL);
        if (0 > dirfd)
            die(EX_SOFTWARE, "PTRACE_PEEKUSER failed: %s", strerror(errno));
        ptrace_get_string(child->pid, 2, pathname, PATH_MAX);

        if (AT_FDCWD != dirfd && '/' != pathname[0]) {
            int n;
            char dname[PATH_MAX], res_dname[PATH_MAX];

            snprintf(dname, PATH_MAX, "/proc/%i/fd/%i", child->pid, dirfd);
            n = readlink(dname, res_dname, PATH_MAX - 1);
            if (0 > n)
                die(EX_SOFTWARE, "readlink failed for %s: %s", dname, strerror(errno));
            res_dname[n] = '\0';

            char *pathc = xstrndup(pathname, PATH_MAX);
            snprintf(pathname, PATH_MAX, "%s/%s", res_dname, pathc);
            free(pathc);
            if (0 > ptrace(PTRACE_POKEUSER, child->pid, PARAM1, AT_FDCWD))
                die(EX_SOFTWARE, "PTRACE_POKEUSER failed: %s", strerror(errno));
        }
    }
    else
        ptrace_get_string(child->pid, arg, pathname, PATH_MAX);

    if (!(sflags & DONT_RESOLV))
        rpath = safe_realpath(pathname, child->pid, 1, &issymlink);
    else
        rpath = safe_realpath(pathname, child->pid, 0, NULL);

    if (NULL == rpath) {
        if (ENOENT == errno) {
            /* File doesn't exist, check the directory */
            char *dirc, *dname;
            dirc = xstrndup(pathname, PATH_MAX);
            dname = dirname(dirc);
            if (!(sflags & DONT_RESOLV))
                rpath = safe_realpath(dname, child->pid, 1, &issymlink);
            else
                rpath = safe_realpath(dname, child->pid, 0, NULL);
            free(dirc);

            lg(LOG_DEBUG, "syscall.syscall_check_path.no_file",
                    "File %s doesn't exist, using directory %s",
                    pathname, rpath);
            if (NULL == rpath) {
                /* Directory doesn't exist as well.
                 * The system call will fail, to prevent any kind of races
                 * we deny access without calling it but don't throw an
                 * access violation.
                 */
                lg(LOG_DEBUG, "syscall.syscall_check_path.no_file_and_dir",
                        "Neither file %s nor directory %s exists, deny access without violation",
                        pathname, rpath);
                decs->res = R_DENY_RETURN;
                decs->ret = -1;
                return 0;
            }
            else {
                lg(LOG_DEBUG, "syscall.syscall_check_path.no_file_but_dir",
                        "File %s doesn't exist but directory %s exists, adding basename",
                        pathname, rpath);
                char *basec, *bname;
                basec = xstrndup(pathname, PATH_MAX);
                bname = basename(basec);
                strncat(rpath, "/", 1);
                strncat(rpath, bname, strlen(bname));
                free(basec);
            }
        }
        else {
            lg(LOG_WARNING, "syscall.syscall_check.safe_realpath_fail",
                    "safe_realpath() failed for \"%s\": %s", pathname, strerror(errno));
            decs->res = R_DENY_RETURN;
            decs->ret = -1;
            return 0;
        }
    }

    if (sflags & ACCESS_MODE) {
        int mode = ptrace(PTRACE_PEEKUSER, child->pid, PARAM2, NULL);
        if (0 > mode) {
            free(rpath);
            die(EX_SOFTWARE, "PTRACE_PEEKUSER failed: %s", strerror(errno));
        }
        if (!(mode & W_OK)) {
            if (issymlink) {
                /* Change the pathname argument with the resolved path to
                 * prevent symlink races.
                 */
                ptrace_set_string(child->pid, 1, rpath, PATH_MAX);
            }
            free(rpath);
            decs->res = R_ALLOW;
            return 0;
        }
    }
    else if (sflags & OPEN_MODE) {
        int mode = ptrace(PTRACE_PEEKUSER, child->pid, PARAM2, NULL);
        if (0 > mode) {
            free(rpath);
            die(EX_SOFTWARE, "PTRACE_PEEKUSER failed: %s", strerror(errno));
        }
        if (!(mode & O_WRONLY || mode & O_RDWR)) {
            if (issymlink) {
                /* Change the pathname argument with the resolved path to
                 * prevent symlink races.
                 */
                ptrace_set_string(child->pid, 1, rpath, PATH_MAX);
            }
            free(rpath);
            decs->res = R_ALLOW;
            return 0;
        }
    }
    else if (sflags & OPEN_MODE_AT) {
        int mode = ptrace(PTRACE_PEEKUSER, child->pid, PARAM3, NULL);
        if (0 > mode) {
            free(rpath);
            die(EX_SOFTWARE, "PTRACE_PEEKUSER failed: %s", strerror(errno));
        }
        if (!(mode & O_WRONLY || mode & O_RDWR)) {
            if (issymlink) {
                /* Change the pathname argument with the resolved path to
                 * prevent symlink races.
                 */
                ptrace_set_string(child->pid, 2, rpath, PATH_MAX);
            }
            free(rpath);
            decs->res = R_ALLOW;
            return 0;
        }
    }

    int allow_write = pathlist_check(&(ctx->write_prefixes), rpath);
    int allow_predict = pathlist_check(&(ctx->predict_prefixes), rpath);

    if (!allow_write && !allow_predict) {
        decs->res = R_DENY_VIOLATION;
        if (1 == arg)
            snprintf(decs->reason, REASON_MAX, "%s(\"%s\", ", sname,
                    pathname);
        else if (2 == arg)
            snprintf(decs->reason, REASON_MAX, "%s(?, \"%s\", ", sname,
                    pathname);
        if (sflags & ACCESS_MODE)
            strcat(decs->reason, "O_WR)");
        else if (sflags & OPEN_MODE || sflags & OPEN_MODE_AT)
            strcat(decs->reason, "O_WRONLY/O_RDWR, ...)");
        else
            strcat(decs->reason, "...)");
        free(rpath);
        return 0;
    }
    else if (!allow_write && allow_predict) {
        if (sflags & RETURNS_FD) {
            /* Change path argument to /dev/null.
             */
            lg(LOG_DEBUG, "syscall.syscall_check_path.devnull_subs",
                    "System call returns fd and its argument is under a predict path");
            lg(LOG_DEBUG, "syscall.syscall_check_path.devnull_subs",
                    "Changing the path argument to /dev/null");
            ptrace_set_string(child->pid, arg, "/dev/null", 10);
            decs->res = R_ALLOW;
        }
        else {
            decs->res = R_DENY_RETURN;
            decs->ret = 0;
        }
        free(rpath);
        return 0;
    }

    if (issymlink) {
        /* Change the pathname argument with the resolved path to
        * prevent symlink races.
        */
        lg(LOG_DEBUG, "syscall.syscall_check_path.resolved_subs",
                "Substituting symlink %s with resolved path %s to prevent races",
                pathname, rpath);
        ptrace_set_string(child->pid, arg, rpath, PATH_MAX);
    }
    free(rpath);
    decs->res = R_ALLOW;
    return 0;
}

struct decision syscall_check(context_t *ctx, struct tchild *child, int syscall) {
    unsigned int sflags, i;
    const char *sname;
    struct decision decs;
    for (i = 0; system_calls[i].name; i++) {
        if (system_calls[i].no == syscall)
            goto found;
    }
    decs.res = R_ALLOW;
    return decs;
found:
    sflags = system_calls[i].flags;
    sname = system_calls[i].name;

    lg(LOG_DEBUG, "syscall.syscall_check.essential",
            "Child %i called essential system call %s()", child->pid, sname);

    if (sflags & CHECK_PATH) {
        syscall_check_path(ctx, child, &decs, 1, sflags, sname);
        if (R_ALLOW != decs.res)
            return decs;
    }
    if (sflags & CHECK_PATH2) {
        syscall_check_path(ctx, child, &decs, 2, sflags, sname);
        if (R_ALLOW != decs.res)
            return decs;
    }
    if (sflags & CHECK_PATH_AT) {
        syscall_check_path(ctx, child, &decs, 2, sflags, sname);
        if (R_ALLOW != decs.res)
            return decs;
    }
    if (sflags & NET_CALL && !(ctx->net_allowed)) {
        decs.res = R_DENY_VIOLATION;
#if defined(I386)
        snprintf(decs.reason, REASON_MAX, "socketcall()");
#elif defined(X86_64)
        snprintf(decs.reason, REASON_MAX, "socket()");
#endif
        decs.ret = -1;
        return decs;
    }
    decs.res = R_ALLOW;
    return decs;
}

int syscall_handle(context_t *ctx, struct tchild *child) {
    int syscall;
    struct decision decs;

    syscall = ptrace_get_syscall(child->pid);
    if (!child->in_syscall) { /* Entering syscall */
#if 0
        lg(LOG_DEBUG, "syscall.syscall_handle.syscall_enter",
                "Child %i is entering system call number %d",
                child->pid, syscall);
#endif
        decs = syscall_check(ctx, child, syscall);
        switch(decs.res) {
            case R_DENY_VIOLATION:
                access_error(child->pid, decs.reason);
                decs.ret = -1;
            case R_DENY_RETURN:
                child->error_code = decs.ret;
                child->orig_syscall = syscall;
                ptrace_set_syscall(child->pid, 0xbadca11);
                break;
            case R_ALLOW:
            default:
                break;
        }
        child->in_syscall = 1;
    }
    else { /* Exiting syscall */
#if 0
        lg(LOG_DEBUG, "syscall.syscall_handle.syscall_exit",
                "Child %i is exiting system call number %d",
                child->pid, syscall);
#endif
        if (0xbadca11 == syscall) {
            /* Restore real call number and return our error code */
            ptrace_set_syscall(child->pid, child->orig_syscall);
            if (0 != ptrace(PTRACE_POKEUSER, child->pid, ACCUM, child->error_code)) {
                lg(LOG_ERROR, "syscall.syscall_handle.fail_pokeuser",
                        "Failed to set error code to %d: %s", child->error_code,
                        strerror(errno));
                return -1;
            }
        }
        child->in_syscall = 0;
    }
    return 0;
}
