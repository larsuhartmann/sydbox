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

static const struct syscall_name {
    int no;
    const char *name;
} sysnames[] = {
#include "syscallent.h"
{-1,    NULL}
};

/* System call dispatch table */
static const struct syscall_def syscalls[] = {
    {__NR_chmod,        CHECK_PATH},
    {__NR_chown,        CHECK_PATH},
#if defined(I386)
    {__NR_chown32,      CHECK_PATH},
#endif
    {__NR_open,         CHECK_PATH | RETURNS_FD | OPEN_MODE | MAGIC_OPEN},
    {__NR_creat,        CHECK_PATH},
    {__NR_stat,         MAGIC_STAT},
    {__NR_lchown,       CHECK_PATH | DONT_RESOLV},
#if defined(I386)
    {__NR_lchown32,     CHECK_PATH | DONT_RESOLV},
#endif
    {__NR_link,         CHECK_PATH},
    {__NR_mkdir,        CHECK_PATH},
    {__NR_mknod,        CHECK_PATH},
    {__NR_access,       CHECK_PATH | ACCESS_MODE},
    {__NR_rename,       CHECK_PATH | CHECK_PATH2},
    {__NR_rmdir,        CHECK_PATH},
    {__NR_symlink,      CHECK_PATH2 | DONT_RESOLV},
    {__NR_truncate,     CHECK_PATH},
#if defined(I386)
    {__NR_truncate64,   CHECK_PATH},
#endif
    {__NR_mount,        CHECK_PATH2},
#if defined(I386)
    {__NR_umount,       CHECK_PATH},
#endif
    {__NR_umount2,      CHECK_PATH},
    {__NR_utime,        CHECK_PATH},
    {__NR_unlink,       CHECK_PATH},
    {__NR_openat,       CHECK_PATH_AT | OPEN_MODE_AT | RETURNS_FD},
    {__NR_mkdirat,      CHECK_PATH_AT},
    {__NR_mknodat,      CHECK_PATH_AT},
    {__NR_fchownat,     CHECK_PATH_AT},
    {__NR_unlinkat,     CHECK_PATH_AT},
    {__NR_renameat,     CHECK_PATH_AT | CHECK_PATH_AT2},
    {__NR_linkat,       CHECK_PATH_AT},
    {__NR_symlinkat,    CHECK_PATH_AT2 | DONT_RESOLV},
    {__NR_fchmodat,     CHECK_PATH_AT},
    {__NR_faccessat,    CHECK_PATH_AT | ACCESS_MODE_AT},
#if defined(I386)
    {__NR_socketcall,   NET_CALL},
#elif defined(X86_64)
    {__NR_socket,       NET_CALL},
#endif
    {-1,                 0}
};

const char *syscall_get_name(int no) {
    for (int i = 0; sysnames[i].name != NULL; i++) {
        if (sysnames[i].no == no)
            return sysnames[i].name;
    }
    return NULL;
}

int syscall_check_prefix(context_t *ctx, struct tchild *child,
        int arg, const struct syscall_def *sdef,
        const char *path, const char *rpath, int issymlink) {
    lg(LOG_DEBUG, "syscall.check.prefix", "Checking \"%s\" for write access", rpath);
    int allow_write = pathlist_check(&(ctx->write_prefixes), rpath);
    lg(LOG_DEBUG, "syscall.check.prefix", "Checking \"%s\" for predict access", rpath);
    int allow_predict = pathlist_check(&(ctx->predict_prefixes), rpath);

    char reason[PATH_MAX + 128];
    const char *sname = syscall_get_name(sdef->no);
    if (!allow_write && !allow_predict) {
        child->retval = -EPERM;
        if (0 == arg)
            strcpy(reason, "%s(\"%s\", ");
        else if (1 == arg)
            strcpy(reason, "%s(?, \"%s\", ");
        if (sdef->flags & ACCESS_MODE)
            strcat(reason, "O_WR)");
        else if (sdef->flags & OPEN_MODE || sdef->flags & OPEN_MODE_AT)
            strcat(reason, "O_WRONLY/O_RDWR)");
        else
            strcat(reason, "...)");
        access_error(child->pid, reason, sname, path);
        return 0;
    }
    else if (!allow_write && allow_predict) {
        if (sdef->flags & RETURNS_FD) {
            lg(LOG_DEBUG, "syscall.check.prefix.subs.devnull",
                    "System call returns fd and its argument is under a predict path");
            lg(LOG_DEBUG, "syscall.check.prefix.subs.devnull",
                    "Changing the path argument to /dev/null");
            if (0 > trace_set_string(child->pid, arg, "/dev/null", 10))
                die(EX_SOFTWARE, "Failed to set string: %s", strerror(errno));
            return 1;
        }
        else {
            child->retval = 0;
            return 0;
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
        if (0 > trace_set_string(child->pid, arg, rpath, PATH_MAX))
            die(EX_SOFTWARE, "Failed to set string: %s", strerror(errno));
    }
    return 1;
}

void syscall_process_pathat(pid_t pid, int arg, char *dest) {
    long dirfd;

    assert(0 == arg || 2 == arg);
    if (0 > trace_get_arg(pid, arg, &dirfd))
        die(EX_SOFTWARE, "Failed to get dirfd: %s", strerror(errno));
    if (0 > trace_get_string(pid, arg + 1, dest, PATH_MAX))
        die(EX_SOFTWARE, "Failed to get string from argument %d: %s",
                arg + 1, strerror(errno));

    if (AT_FDCWD != dirfd && '/' != dest[0]) {
        int n;
        char dname[PATH_MAX], res_dname[PATH_MAX];

        snprintf(dname, PATH_MAX, "/proc/%i/fd/%ld", pid, dirfd);
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
    long mode;
    if (sdef->flags & ACCESS_MODE) {
        if (0 > trace_get_arg(pid, 1, &mode)) {
            lg(LOG_ERROR, "syscall.check.access.mode.fail",
                    "Failed to get mode from argument 1: %s",
                    strerror(errno));
            return -1;
        }
    }
    else { /* if (sdef->flags & ACCESS_MODE_AT) */
        if (0 > trace_get_arg(pid, 2, &mode)) {
            lg(LOG_ERROR, "syscall.check.access.mode.fail",
                    "Failed to get mode from argument 2: %s",
                    strerror(errno));
            return -1;
        }
    }

    if (!(mode & W_OK)) {
        if (issymlink) {
            lg(LOG_DEBUG, "syscall.check.access.subs.resolved",
                "Substituting symlink \"%s\" with resolved path \"%s\" to prevent races",
                path, rpath);
            if (sdef->flags & ACCESS_MODE) {
                if (trace_set_string(pid, 0, rpath, PATH_MAX))
                    die(EX_SOFTWARE, "Failed to set string: %s", strerror(errno));
            }
            else {
                if (0 > trace_set_string(pid, 1, rpath, PATH_MAX))
                    die(EX_SOFTWARE, "Failed to set string: %s", strerror(errno));
            }
        }
        return 1;
    }
    return 0;
}

int syscall_check_open(pid_t pid, const char *path, const char *rpath, int issymlink) {
    long mode;

    if (0 > trace_get_arg(pid, 1, &mode)) {
        lg(LOG_ERROR, "syscall.check.open.mode.fail",
                "Failed to get mode: %s", strerror(errno));
        return -1;
    }
    if (!(mode & O_WRONLY || mode & O_RDWR)) {
        if (issymlink) {
            lg(LOG_DEBUG, "syscall.check.open.subs.resolved",
                "Substituting symlink \"%s\" with resolved path \"%s\" to prevent races",
                path, rpath);
            if (0 > trace_set_string(pid, 0, rpath, PATH_MAX))
                die(EX_SOFTWARE, "Failed to set string: %s", strerror(errno));
        }
        return 1;
    }
    return 0;
}

int syscall_check_openat(pid_t pid, const char *path, const char *rpath, int issymlink) {
    long mode;

    if (0 > trace_get_arg(pid, 1, &mode)) {
        lg(LOG_ERROR, "syscall.check.openat.mode.fail",
                "Failed to get mode: %s", strerror(errno));
        return -1;
    }
    if (!(mode & O_WRONLY || mode & O_RDWR)) {
        if (issymlink) {
            lg(LOG_DEBUG, "syscall.check.open.subs.resolved",
                "Substituting symlink \"%s\" with resolved path \"%s\" to prevent races",
                path, rpath);
            if (0 > trace_set_string(pid, 1, rpath, PATH_MAX))
                die(EX_SOFTWARE, "Failed to set string: %s", strerror(errno));
        }
        return 1;
    }
    return 0;
}

int syscall_check_path(context_t *ctx, struct tchild *child,
        int arg, const struct syscall_def *sdef) {
    int issymlink;
    char *rpath;
    char path[PATH_MAX];

    assert(0 == arg || 1 == arg);

    if (sdef->flags & CHECK_PATH || sdef->flags & CHECK_PATH2) {
        if (0 > trace_get_string(child->pid, arg, path, PATH_MAX))
            die(EX_SOFTWARE, "Failed to get string from argument %d: %s",
                    arg, strerror(errno));
    }
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
            return 1;
        }
        else if (0 != errno) {
            /* safe_realpath() failed */
            child->retval = -errno;
            return 0;
        }
    }

    int ret, check_ret = 0;
    if (sdef->flags & ACCESS_MODE || sdef->flags & ACCESS_MODE_AT) {
        check_ret = 1;
        ret = syscall_check_access(child->pid, sdef, path, rpath, issymlink);
    }
    else if (sdef->flags & OPEN_MODE) {
        check_ret = 1;
        ret = syscall_check_open(child->pid, path, rpath, issymlink);
    }
    else if (sdef->flags & OPEN_MODE_AT) {
        check_ret = 1;
        ret = syscall_check_openat(child->pid, path, rpath, issymlink);
    }

    if (check_ret) {
        if (0 > ret) {
            free(rpath);
            die(EX_SOFTWARE, "ptrace: %s", strerror(errno));
        }
        else if (ret) { /* W_OK or O_WRONLY and O_RDWR not in flags */
            free(rpath);
            return 1;
        }
    }

    ret = syscall_check_prefix(ctx, child, arg, sdef, path, rpath, issymlink);
    free(rpath);
    return ret;
}

int syscall_check_magic_open(context_t *ctx, struct tchild *child) {
    char pathname[PATH_MAX];
    const char *rpath;

    if (0 > trace_get_string(child->pid, 0, pathname, PATH_MAX))
        die(EX_SOFTWARE, "Failed to get string from argument 0: %s", strerror(errno));
    lg(LOG_DEBUG, "syscall.check.magic.open.ismagic",
            "Checking if open(\"%s\", ...) is magic", pathname);
    if (path_magic_write(pathname)) {
        rpath = pathname + CMD_WRITE_LEN - 1;
        if (context_cmd_allowed(ctx, child)) {
            lg(LOG_NORMAL, "syscall.check_magic.write.allow",
                    "Approved addwrite(\"%s\") for child %i", rpath, child->pid);
            pathnode_new(&(ctx->write_prefixes), rpath);
            /* Change argument to /dev/null */
            lg(LOG_DEBUG, "syscall.check.magic.write.devnull",
                    "Changing pathname to /dev/null");
            if (0 > trace_set_string(child->pid, 0, "/dev/null", 10))
                die(EX_SOFTWARE, "Failed to set string: %s", strerror(errno));
            return 1;
        }
        else {
            lg(LOG_WARNING, "syscall.check.magic.write.deny",
                    "Denied addwrite(\"%s\") for child %i", rpath, child->pid);
            return 0;
        }
    }
    else if (path_magic_predict(pathname)) {
        rpath = pathname + CMD_PREDICT_LEN - 1;
        if (context_cmd_allowed(ctx, child)) {
            lg(LOG_NORMAL, "syscall.check.magic.predict.allow",
                    "Approved addpredict(\"%s\") for child %i", rpath, child->pid);
            pathnode_new(&(ctx->predict_prefixes), rpath);
            /* Change argument to /dev/null */
            lg(LOG_DEBUG, "syscall.check.magic.predict.devnull",
                    "Changing pathname to /dev/null");
            if (0 > trace_set_string(child->pid, 0, "/dev/null", 10))
                die(EX_SOFTWARE, "Failed to set string: %s", strerror(errno));
            return 1;
        }
        else {
            lg(LOG_WARNING, "syscall.check.magic.predict.deny",
                    "Denied addpredict(\"%s\") for child %i", rpath, child->pid);
            return 0;
        }
    }
    lg(LOG_DEBUG, "syscall.check.magic.open.nonmagic",
            "open(\"%s\", ...) not magic", pathname);
    return 0;
}

int syscall_check_magic_stat(struct tchild *child) {
    char pathname[PATH_MAX];

    if (0 > trace_get_string(child->pid, 0, pathname, PATH_MAX))
        die(EX_SOFTWARE, "Failed to get string from argument 0: %s", strerror(errno));
    lg(LOG_DEBUG, "syscall.check.magic.stat.ismagic",
            "Checking if stat(\"%s\") is magic", pathname);
    if (path_magic_dir(pathname)) {
        lg(LOG_DEBUG, "syscall.check.magic.stat.magic",
                "stat(\"%s\") is magic", pathname);
        return 1;
    }
    else {
        lg(LOG_DEBUG, "syscall.check.magic.stat.nonmagic",
                "stat(\"%s\") is not magic", pathname);
        return 0;
    }
}

int syscall_check(context_t *ctx, struct tchild *child, int syscall) {
    unsigned int i;
    const char *sname;
    const struct syscall_def *sdef;
    for (i = 0; syscalls[i].no != -1; i++) {
        if (syscalls[i].no == syscall)
            goto found;
    }
    return 1;
found:
    sdef = &(syscalls[i]);
    sname = syscall_get_name(sdef->no);

    lg(LOG_DEBUG, "syscall.check.essential",
            "Child %i called essential system call %s()", child->pid, sname);

    /* Handle magic calls */
    if (sdef->flags & MAGIC_OPEN && syscall_check_magic_open(ctx, child))
        return 1;
    else if (sdef->flags & MAGIC_STAT) {
        if(syscall_check_magic_stat(child)) {
            child->retval = 0;
            return 0;
        }
    }

    if (sdef->flags & CHECK_PATH) {
        lg(LOG_DEBUG, "syscall.check.check_path",
                "System call %s() has CHECK_PATH set, checking", sname);
        /* Return here only if access is denied because some syscalls have
         * both CHECK_PATH and CHECK_PATH2 set.
         */
        if (!syscall_check_path(ctx, child, 0, sdef))
            return 0;
    }
    if (sdef->flags & CHECK_PATH2) {
        lg(LOG_DEBUG, "syscall.check.checkpath2",
                "System call %s() has CHECK_PATH2 set, checking", sname);
        return syscall_check_path(ctx, child, 1, sdef);
    }
    if (sdef->flags & CHECK_PATH_AT) {
        lg(LOG_DEBUG, "syscall.check.check_path_at",
                "System call %s() has CHECK_PATH_AT set, checking", sname);
        return syscall_check_path(ctx, child, 1, sdef);
    }
    if (sdef->flags & NET_CALL && !(ctx->net_allowed)) {
#if defined(I386)
        access_error(child->pid, "socketcall()");
#elif defined(X86_64)
        access_error(child->pid, "socket()");
#endif
        child->retval = -EACCES;
        return 0;
    }
    return 1;
}

int syscall_handle(context_t *ctx, struct tchild *child) {
    long syscall;
    const char *sname;

    if (0 > trace_get_syscall(child->pid, &syscall))
        die(EX_SOFTWARE, "Failed to get syscall: %s", strerror(errno));
    sname = syscall_get_name(syscall);
    if (!(child->flags & TCHILD_INSYSCALL)) { /* Entering syscall */
        lg(LOG_DEBUG_CRAZY, "syscall.handle.enter",
                "Child %i is entering system call %s()",
                child->pid, sname);
        if (!syscall_check(ctx, child, syscall)) {
            /* Deny access */
            child->syscall = syscall;
            if (0 > trace_set_syscall(child->pid, 0xbadca11))
                die(EX_SOFTWARE, "Failed to set syscall: %s", strerror(errno));
        }
        child->flags ^= TCHILD_INSYSCALL;
    }
    else { /* Exiting syscall */
        lg(LOG_DEBUG_CRAZY, "syscall.handle.exit",
                "Child %i is exiting system call %s()",
                child->pid, sname);
        if (0xbadca11 == syscall) {
            /* Restore real call number and return our error code */
            if (0 > trace_set_syscall(child->pid, child->syscall))
                die(EX_SOFTWARE, "Failed to restore syscall: %s", strerror(errno));
            if (0 > trace_set_return(child->pid, child->retval))
                die(EX_SOFTWARE, "Failed to set return code: %s", strerror(errno));
        }
        child->flags ^= TCHILD_INSYSCALL;
    }
    return 0;
}
