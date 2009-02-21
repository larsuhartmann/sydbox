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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <sys/stat.h>
#include <sys/ptrace.h>

#include "defs.h"

/* System call dispatch flags */
#define RETURNS_FD      (1 << 0) /* The function returns a file descriptor */
#define OPEN_MODE       (1 << 1) /* Check the mode argument of open() */
#define OPEN_MODE_AT    (1 << 2) /* Check the mode argument of openat() */
#define ACCESS_MODE     (1 << 3) /* Check the mode argument of access() */
#define ACCESS_MODE_AT  (1 << 4) /* Check the mode argument of faccessat() */
#define CHECK_PATH      (1 << 5) /* First argument should be a valid path */
#define CHECK_PATH2     (1 << 6) /* Second argument should be a valid path */
#define CHECK_PATH_AT   (1 << 7) /* CHECK_PATH for at suffixed functions */
#define CHECK_PATH_AT2  (1 << 8) /* CHECK_PATH2 for at suffixed functions */
#define DONT_RESOLV     (1 << 9) /* Don't resolve symlinks */
#define MAGIC_OPEN      (1 << 10) /* Check if the open() call is magic */
#define MAGIC_STAT      (1 << 11) /* Check if the stat() call is magic */
#define NET_CALL        (1 << 12) /* Allowing the system call depends on the net flag */

/* System call dispatch table */
const struct syscall_def system_calls[] = {
    {__NR_chmod,        "chmod",        CHECK_PATH},
    {__NR_chown,        "chown",        CHECK_PATH},
#if defined(I386)
    {__NR_chown32,      "chown32",      CHECK_PATH},
#endif
    {__NR_open,         "open",         CHECK_PATH | RETURNS_FD | OPEN_MODE | MAGIC_OPEN},
    {__NR_creat,        "creat",        CHECK_PATH},
    {__NR_stat,         "stat",         MAGIC_STAT},
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
    {__NR_mount,        "mount",        CHECK_PATH2},
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
    {__NR_renameat,     "renameat",     CHECK_PATH_AT | CHECK_PATH_AT2},
    {__NR_linkat,       "linkat",       CHECK_PATH_AT},
    {__NR_symlinkat,    "symlinkat",    CHECK_PATH_AT2 | DONT_RESOLV},
    {__NR_fchmodat,     "fchmodat",     CHECK_PATH_AT},
    {__NR_faccessat,    "faccessat",    CHECK_PATH_AT | ACCESS_MODE_AT},
#if defined(I386)
    {__NR_socketcall,   "socketcall",   NET_CALL},
#elif defined(X86_64)
    {__NR_socket,       "socket",       NET_CALL},
#endif
    {0,                 NULL,           0}
};

int syscall_check_prefix(context_t *ctx, pid_t pid, int arg, const struct syscall_def *sdef,
        const char *path, const char *rpath, int issymlink, struct decision *decs) {
    lg(LOG_DEBUG, "syscall.check.prefix", "Checking \"%s\" for write access", rpath);
    int allow_write = pathlist_check(&(ctx->write_prefixes), rpath);
    lg(LOG_DEBUG, "syscall.check.prefix", "Checking \"%s\" for predict access", rpath);
    int allow_predict = pathlist_check(&(ctx->predict_prefixes), rpath);

    if (!allow_write && !allow_predict) {
        decs->res = R_DENY_VIOLATION;
        if (0 == arg)
            snprintf(decs->reason, REASON_MAX, "%s(\"%s\", ", sdef->name, path);
        else if (1 == arg)
            snprintf(decs->reason, REASON_MAX, "%s(?, \"%s\", ", sdef->name, path);
        if (sdef->flags & ACCESS_MODE)
            strcat(decs->reason, "O_WR)");
        else if (sdef->flags & OPEN_MODE || sdef->flags & OPEN_MODE_AT)
            strcat(decs->reason, "O_WRONLY/O_RDWR, ...)");
        else
            strcat(decs->reason, "...)");
        return 0;
    }
    else if (!allow_write && allow_predict) {
        if (sdef->flags & RETURNS_FD) {
            lg(LOG_DEBUG, "syscall.check.prefix.subs.devnull",
                    "System call returns fd and its argument is under a predict path");
            lg(LOG_DEBUG, "syscall.check.prefix.subs.devnull",
                    "Changing the path argument to /dev/null");
            ptrace_set_string(pid, arg, "/dev/null", 10);
            decs->res = R_ALLOW;
        }
        else {
            decs->res = R_DENY_RETURN;
            decs->ret = 0;
        }
        return 0;
    }

    if (issymlink) {
        /* Change the pathname argument with the resolved path to
        * prevent symlink races.
        */
        lg(LOG_DEBUG, "syscall.check.prefix.subs.resolved",
                "Substituting symlink %s with resolved path %s to prevent races",
                path, rpath);
        ptrace_set_string(pid, arg, rpath, PATH_MAX);
    }
    decs->res = R_ALLOW;
    return 0;
}

void syscall_process_pathat(pid_t pid, int arg, char *dest) {
    int dirfd;

    assert(0 == arg || 2 == arg);
    errno = 0;
    dirfd = ptrace(PTRACE_PEEKUSER, pid, syscall_args[arg], NULL);
    if (0 != errno)
        die(EX_SOFTWARE, "PTRACE_PEEKUSER failed: %s", strerror(errno));
    ptrace_get_string(pid, arg + 1, dest, PATH_MAX);

    if (AT_FDCWD != dirfd && '/' != dest[0]) {
        int n;
        char dname[PATH_MAX], res_dname[PATH_MAX];

        snprintf(dname, PATH_MAX, "/proc/%i/fd/%i", pid, dirfd);
        n = readlink(dname, res_dname, PATH_MAX - 1);
        if (0 > n)
            die(EX_SOFTWARE, "readlink failed for %s: %s", dname, strerror(errno));
        res_dname[n] = '\0';

        char *destc = xstrndup(dest, PATH_MAX);
        snprintf(dest, PATH_MAX, "%s/%s", res_dname, destc);
        free(destc);
    }
}

int syscall_check_access(pid_t pid, const struct syscall_def *sdef,
        const char *path, const char *rpath, int issymlink) {
    int mode;
    errno = 0;
    if (sdef->flags & ACCESS_MODE)
        mode = ptrace(PTRACE_PEEKUSER, pid, syscall_args[1], NULL);
    else /* if (sdef->flags & ACCESS_MODE_AT) */
        mode = ptrace(PTRACE_PEEKUSER, pid, syscall_args[2], NULL);

    if (0 != errno) {
        int save_errno = errno;
        lg(LOG_ERROR, "syscall.check.access",
                "PTRACE_PEEKUSER failed: %s", strerror(errno));
        errno = save_errno;
        return -1;
    }

    if (!(mode & W_OK)) {
        if (issymlink) {
            lg(LOG_DEBUG, "syscall.check.access.subs.resolved",
                "Substituting symlink \"%s\" with resolved path \"%s\" to prevent races",
                path, rpath);
            if (sdef->flags & ACCESS_MODE)
                ptrace_set_string(pid, 0, rpath, PATH_MAX);
            else
                ptrace_set_string(pid, 1, rpath, PATH_MAX);
        }
        return 1;
    }
    return 0;
}

int syscall_check_open(pid_t pid, const char *path, const char *rpath, int issymlink) {
    errno = 0;
    int mode = ptrace(PTRACE_PEEKUSER, pid, syscall_args[1], NULL);
    if (0 != errno) {
        int save_errno = errno;
        lg(LOG_ERROR, "syscall.check.open",
                "PTRACE_PEEKUSER failed: %s", strerror(errno));
        errno = save_errno;
        return -1;
    }
    if (!(mode & O_WRONLY || mode & O_RDWR)) {
        if (issymlink) {
            lg(LOG_DEBUG, "syscall.check.open.subs.resolved",
                "Substituting symlink \"%s\" with resolved path \"%s\" to prevent races",
                path, rpath);
            ptrace_set_string(pid, 0, rpath, PATH_MAX);
        }
        return 1;
    }
    return 0;
}

int syscall_check_openat(pid_t pid, const char *path, const char *rpath, int issymlink) {
    errno = 0;
    int mode = ptrace(PTRACE_PEEKUSER, pid, syscall_args[2], NULL);
    if (0 != errno) {
        int save_errno = errno;
        lg(LOG_ERROR, "syscall.check.openat",
                "PTRACE_PEEKUSER failed: %s", strerror(errno));
        errno = save_errno;
        return -1;
    }
    if (!(mode & O_WRONLY || mode & O_RDWR)) {
        if (issymlink) {
            lg(LOG_DEBUG, "syscall.check.open.subs.resolved",
                "Substituting symlink \"%s\" with resolved path \"%s\" to prevent races",
                path, rpath);
            ptrace_set_string(pid, 1, rpath, PATH_MAX);
        }
        return 1;
    }
    return 0;
}

int syscall_check_path(context_t *ctx, struct tchild *child,
        int arg, const struct syscall_def *sdef, struct decision *decs) {
    int issymlink;
    char *rpath;
    char path[PATH_MAX];

    assert(0 == arg || 1 == arg);

    if (sdef->flags & CHECK_PATH || sdef->flags & CHECK_PATH2)
        ptrace_get_string(child->pid, arg, path, PATH_MAX);
    if (sdef->flags & CHECK_PATH_AT)
        syscall_process_pathat(child->pid, 0, path);
    if (sdef->flags & CHECK_PATH_AT2)
        syscall_process_pathat(child->pid, 2, path);

    if (!(sdef->flags & DONT_RESOLV))
        rpath = resolve_path(path, child->pid, 1, &issymlink);
    else
        rpath = resolve_path(path, child->pid, 0, NULL);

    if (NULL == rpath) {
        if (ENOENT == errno) {
            /* Neither directory nor file exists, allow access
             * XXX This opens a hole for race conditions,
             * but denying access here makes tar fail.
             */
            lg(LOG_DEBUG, "syscall.check_path.file_dir.none",
                    "Neither file nor directory exists, allowing access");
            decs->res = R_ALLOW;
            return 0;
        }
        else if (0 != errno) {
            /* safe_realpath() failed */
            decs->res = R_DENY_RETURN;
            decs->ret = -1;
            return 0;
        }
    }

    if (sdef->flags & ACCESS_MODE || sdef->flags & ACCESS_MODE_AT) {
        int ret = syscall_check_access(child->pid, sdef, path, rpath, issymlink);
        if (0 > ret) {
            free(rpath);
            die(EX_SOFTWARE, "PTRACE_PEEKUSER failed: %s", strerror(errno));
        }
        else if (ret) { /* W_OK not in flags */
            free(rpath);
            decs->res = R_ALLOW;
            return ret;
        }
    }
    else if (sdef->flags & OPEN_MODE) {
        int ret = syscall_check_open(child->pid, path, rpath, issymlink);
        if (0 > ret) {
            free(rpath);
            die(EX_SOFTWARE, "PTRACE_PEEKUSER failed: %s", strerror(errno));
        }
        else if (ret) { /* O_WRONLY or O_RDWR not in flags */
            free(rpath);
            decs->res = R_ALLOW;
            return ret;
        }
    }
    else if (sdef->flags & OPEN_MODE_AT) {
        int ret = syscall_check_openat(child->pid, path, rpath, issymlink);
        if (0 > ret) {
            free(rpath);
            die(EX_SOFTWARE, "PTRACE_PEEKUSER failed: %s", strerror(errno));
        }
        else if (ret) { /* O_WRONLY or O_RDWR not in flags */
            free(rpath);
            decs->res = R_ALLOW;
            return ret;
        }
    }

    int ret = syscall_check_prefix(ctx, child->pid, arg, sdef, path, rpath, issymlink, decs);
    free(rpath);
    return ret;
}

int syscall_check_magic_open(context_t *ctx, struct tchild *child) {
    char pathname[PATH_MAX];
    const char *rpath;

    ptrace_get_string(child->pid, 0, pathname, PATH_MAX);
    if (path_magic_write(pathname)) {
        rpath = pathname + CMD_WRITE_LEN - 1;
        if (context_cmd_allowed(ctx, child)) {
            lg(LOG_NORMAL, "syscall.check_magic.write.allow",
                    "Approved addwrite(\"%s\") for child %i", rpath, child->pid);
            pathnode_new(&(ctx->write_prefixes), rpath);
            /* Change argument to /dev/null */
            lg(LOG_DEBUG, "syscall.check_magic.write.devnull",
                    "Changing pathname to /dev/null");
            ptrace_set_string(child->pid, 0, "/dev/null", 10);
            return 1;
        }
        else
            lg(LOG_WARNING, "syscall.check_magic.write.deny",
                    "Denied addwrite(\"%s\") for child %i", rpath, child->pid);
    }
    else if (path_magic_predict(pathname)) {
        rpath = pathname + CMD_PREDICT_LEN - 1;
        if (context_cmd_allowed(ctx, child)) {
            lg(LOG_NORMAL, "syscall.check_magic.predict.allow",
                    "Approved addpredict(\"%s\") for child %i", rpath, child->pid);
            pathnode_new(&(ctx->predict_prefixes), rpath);
            /* Change argument to /dev/null */
            lg(LOG_DEBUG, "syscall.check_magic.predict.devnull",
                    "Changing pathname to /dev/null");
            ptrace_set_string(child->pid, 0, "/dev/null", 10);
            return 1;
        }
        else
            lg(LOG_WARNING, "syscall.check_magic.predict.deny",
                    "Denied addpredict(\"%s\") for child %i", rpath, child->pid);
    }
    return 0;
}

int syscall_check_magic_stat(struct tchild *child) {
    char pathname[PATH_MAX];

    ptrace_get_string(child->pid, 0, pathname, PATH_MAX);
    if (path_magic_dir(pathname))
        return 1;
    else
        return 0;
}

struct decision syscall_check(context_t *ctx, struct tchild *child, int syscall) {
    unsigned int i;
    struct decision decs;
    const struct syscall_def *sdef;
    for (i = 0; system_calls[i].name; i++) {
        if (system_calls[i].no == syscall)
            goto found;
    }
    decs.res = R_ALLOW;
    return decs;
found:
    sdef = &(system_calls[i]);

    lg(LOG_DEBUG, "syscall.check.essential",
            "Child %i called essential system call %s()", child->pid, sdef->name);

    /* Handle magic calls */
    if (sdef->flags & MAGIC_OPEN) {
        lg(LOG_DEBUG, "syscall.check.open.ismagic", "Checking if open() is magic");
        if (syscall_check_magic_open(ctx, child)) {
            lg(LOG_DEBUG, "syscall.check.open.magic", "Handled magic open() call");
            decs.res = R_ALLOW;
            return decs;
        }
        else
            lg(LOG_DEBUG, "syscall.check.open.nonmagic", "open() not magic");
    }
    else if (sdef->flags & MAGIC_STAT) {
        lg(LOG_DEBUG, "syscall.check.stat.ismagic", "Checking if stat() is magic");
        if(syscall_check_magic_stat(child)) {
            lg(LOG_DEBUG, "syscall.check.stat.magic", "Handled magic stat() call");
            decs.res = R_DENY_RETURN;
            decs.ret = 0;
            return decs;
        }
        else
            lg(LOG_DEBUG, "syscall.check.stat.nonmagic", "stat() not magic");
    }

    if (sdef->flags & CHECK_PATH) {
        lg(LOG_DEBUG, "syscall.check.check_path",
                "System call %s() has CHECK_PATH set, checking", sdef->name);
        syscall_check_path(ctx, child, 0, sdef, &decs);
        switch(decs.res) {
            case R_DENY_VIOLATION:
                lg(LOG_DEBUG, "syscall.check.check_path.deny",
                        "Access denied for system call %s()", sdef->name);
                return decs;
            case R_DENY_RETURN:
                lg(LOG_DEBUG, "syscall.check.check_path.predict",
                        "Access predicted for system call %s()", sdef->name);
                break;
            case R_ALLOW:
            default:
                lg(LOG_DEBUG, "syscall.check.check_path.allow",
                        "Access allowed for system call %s()", sdef->name);
                break;
        }
    }
    if (sdef->flags & CHECK_PATH2) {
        lg(LOG_DEBUG, "syscall.check.checkpath2",
                "System call %s() has CHECK_PATH2 set, checking", sdef->name);
        syscall_check_path(ctx, child, 1, sdef, &decs);
        switch(decs.res) {
            case R_DENY_VIOLATION:
                lg(LOG_DEBUG, "syscall.check.check_path2.deny",
                        "Access denied for system call %s()", sdef->name);
                return decs;
            case R_DENY_RETURN:
                lg(LOG_DEBUG, "syscall.check.check_path2.predict",
                        "Access predicted for system call %s()", sdef->name);
                break;
            case R_ALLOW:
            default:
                lg(LOG_DEBUG, "syscall.check.check_path2.allow",
                        "Access allowed for system call %s()", sdef->name);
                break;
        }
    }
    if (sdef->flags & CHECK_PATH_AT) {
        lg(LOG_DEBUG, "syscall.check.check_path_at",
                "System call %s() has CHECK_PATH_AT set, checking", sdef->name);
        syscall_check_path(ctx, child, 1, sdef, &decs);
        switch(decs.res) {
            case R_DENY_VIOLATION:
                lg(LOG_DEBUG, "syscall.check.check_path_at.deny",
                        "Access denied for system call %s()", sdef->name);
                return decs;
            case R_DENY_RETURN:
                lg(LOG_DEBUG, "syscall.check.check_path_at.predict",
                        "Access predicted for system call %s()", sdef->name);
                break;
            case R_ALLOW:
            default:
                lg(LOG_DEBUG, "syscall.check.check_path_at.allow",
                        "Access allowed for system call %s()", sdef->name);
                break;
        }
    }
    if (sdef->flags & NET_CALL && !(ctx->net_allowed)) {
        decs.res = R_DENY_VIOLATION;
#if defined(I386)
        snprintf(decs.reason, REASON_MAX, "socketcall()");
#elif defined(X86_64)
        snprintf(decs.reason, REASON_MAX, "socket()");
#endif
        decs.ret = -1;
        return decs;
    }
    return decs;
}

int syscall_handle(context_t *ctx, struct tchild *child) {
    int syscall;
    struct decision decs;

    syscall = ptrace_get_syscall(child->pid);
    if (!child->in_syscall) { /* Entering syscall */
        lg(LOG_DEBUG_CRAZY, "syscall.handle.syscall_enter",
                "Child %i is entering system call number %d",
                child->pid, syscall);
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
        lg(LOG_DEBUG_CRAZY, "syscall.handle.syscall_exit",
                "Child %i is exiting system call number %d",
                child->pid, syscall);
        if (0xbadca11 == syscall) {
            /* Restore real call number and return our error code */
            ptrace_set_syscall(child->pid, child->orig_syscall);
            if (0 != ptrace(PTRACE_POKEUSER, child->pid, ACCUM, child->error_code)) {
                lg(LOG_ERROR, "syscall.handle.err.fail.pokeuser",
                        "Failed to set error code to %d: %s", child->error_code,
                        strerror(errno));
                return -1;
            }
        }
        child->in_syscall = 0;
    }
    return 0;
}
