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
#define ACCESS_MODE     (1 << 2) /* Check the mode argument of access() */
#define CHECK_PATH      (1 << 3) /* First argument should be a valid path */
#define RESOLV_PATH     (1 << 4) /* Resolve first pathname and modify syscall */

/* System call dispatch table */
static struct syscall_def {
    int no;
    const char *name;
    unsigned int flags;
} system_calls[] = {
    {__NR_chmod,        "chmod",        CHECK_PATH | RESOLV_PATH},
    {__NR_chown,        "chown",        CHECK_PATH | RESOLV_PATH},
    {__NR_open,         "open",         CHECK_PATH | RESOLV_PATH | RETURNS_FD | OPEN_MODE},
    {__NR_creat,        "creat",        CHECK_PATH | RESOLV_PATH},
    {__NR_lchown,       "lchown",       CHECK_PATH},
    {__NR_link,         "link",         CHECK_PATH | RESOLV_PATH},
    {__NR_mkdir,        "mkdir",        CHECK_PATH | RESOLV_PATH},
    {__NR_mknod,        "mknod",        CHECK_PATH | RESOLV_PATH},
    {__NR_access,       "access",       CHECK_PATH | RESOLV_PATH | ACCESS_MODE},
    {__NR_rename,       "rename",       CHECK_PATH | RESOLV_PATH},
    {__NR_rmdir,        "rmdir",        CHECK_PATH | RESOLV_PATH},
    {__NR_symlink,      "symlink",      CHECK_PATH | RESOLV_PATH},
    {__NR_truncate,     "truncate",     CHECK_PATH | RESOLV_PATH},
    {__NR_unlink,       "unlink",       CHECK_PATH},
    {__NR_openat,       "openat",       0},
    {__NR_mkdirat,      "mkdirat",      0},
    {__NR_mknodat,      "mknodat",      0},
    {__NR_fchownat,     "fchownat",     0},
    {__NR_unlinkat,     "unlinkat",     0},
    {__NR_renameat,     "renameat",     0},
    {__NR_linkat,       "linkat",       0},
    {__NR_symlinkat,    "symlinkat",    0},
    {__NR_fchmodat,     "fchmodat",     0},
    {__NR_faccessat,    "faccessat",    0},
    {0,                 NULL,           0}
};

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

    lg(LOG_VERBOSE, "syscall.syscall_check.essential",
            "Child %i called essential system call %s()", child->pid, sname);

    if (sflags & CHECK_PATH) {
        int issymlink;
        char pathname[PATH_MAX];
        char *rpath;

        ptrace_get_string(child->pid, 1, pathname, PATH_MAX);

        if (sflags & RESOLV_PATH)
            rpath = safe_realpath(pathname, child->pid, 1, &issymlink);
        else
            rpath = safe_realpath(pathname, child->pid, 0, NULL);

        if (NULL == rpath) {
            if (ENOENT == errno) {
                /* File doesn't exist, check the directory */
                char *dirc, *dname;
                dirc = xstrndup(pathname, PATH_MAX);
                dname = dirname(dirc);
                if (sflags & RESOLV_PATH)
                    rpath = safe_realpath(dname, child->pid, 1, &issymlink);
                else
                    rpath = safe_realpath(dname, child->pid, 0, NULL);
                free(dirc);

                if (NULL == rpath) {
                    /* Directory doesn't exist as well.
                     * The system call will fail, to prevent any kind of races
                     * we deny access without calling it but don't throw an
                     * access violation.
                     */
                    decs.res = R_DENY_RETURN;
                    decs.ret = -1;
                    return decs;
                }
                else {
                    /* Add the basename back */
                    char *basec, *bname;
                    basec = xstrndup(pathname, PATH_MAX);
                    bname = basename(basec);
                    strncat(rpath, "/", 1);
                    strncat(rpath, bname, strlen(bname));
                    free(basec);
                }
            }
            else {
                lg(LOG_WARNING, "syscall.syscall_check.fail_safe_realpath",
                        "safe_realpath() failed for \"%s\": %s", pathname, strerror(errno));
                decs.res = R_DENY_RETURN;
                decs.ret = -1;
                return decs;
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
                decs.res = R_ALLOW;
                return decs;
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
                decs.res = R_ALLOW;
                return decs;
            }
        }

        int allow_write = pathlist_check(&(ctx->write_prefixes), rpath);
        int allow_predict = pathlist_check(&(ctx->predict_prefixes), rpath);

        if (!allow_write && !allow_predict) {
            decs.res = R_DENY_VIOLATION;
            snprintf(decs.reason, REASON_MAX, "%s(\"%s\", ", sname,
                    pathname);
            if (sflags & ACCESS_MODE)
                strcat(decs.reason, "O_WR)");
            else if (sflags & OPEN_MODE)
                strcat(decs.reason, "O_WRONLY/O_RDWR, ...)");
            else
                strcat(decs.reason, "...)");
            free(rpath);
            return decs;
        }
        else if (!allow_write && allow_predict) {
            if (sflags & RETURNS_FD) {
                /* Change path argument to /dev/null.
                 * TODO: remove O_CREAT from flags if OPEN_MODE is set.
                 */
                ptrace_set_string(child->pid, 1, "/dev/null", 10);
                decs.res = R_ALLOW;
            }
            else {
                decs.res = R_DENY_RETURN;
                decs.ret = 0;
            }
            free(rpath);
            return decs;
        }

        if (issymlink) {
            /* Change the pathname argument with the resolved path to
            * prevent symlink races.
            */
            ptrace_set_string(child->pid, 1, rpath, PATH_MAX);
        }
        free(rpath);
    }
    decs.res = R_ALLOW;
    return decs;
}

int syscall_handle(context_t *ctx, struct tchild *child) {
    int syscall;
    struct decision decs;

    syscall = ptrace_get_syscall(child->pid);
    if (!child->in_syscall) { /* Entering syscall */
        lg(LOG_DEBUG, "syscall.syscall_handle.syscall_enter",
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
        lg(LOG_DEBUG, "syscall.syscall_handle.syscall_exit",
                "Child %i is exiting system call number %d",
                child->pid, syscall);
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
