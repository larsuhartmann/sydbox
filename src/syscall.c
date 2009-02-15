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

#ifndef _ATFILE_SOURCE
#define _ATFILE_SOURCE 1 /* Get AT_* constants */
#endif

#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/ptrace.h>
#include <asm/unistd.h>

#include "defs.h"

/* System call dispatch flags */
#define CHECK_PATH      (1 << 0) /* First argument should be a valid path */
#define CHECK_PATH2     (1 << 1) /* Second argument should be a valid path */
#define CHECK_PATH_AT   (1 << 2) /* First argument is dirfd and second is path */
#define CHECK_PATH_AT2  (1 << 3) /* Third argument is dirfd and fourth is path */
#define RESOLV_PATH     (1 << 4) /* Resolve first pathname and modify syscall */
#define RESOLV_PATH2    (1 << 5) /* Resolve second pathname and modify syscall */
#define RESOLV_PATH_AT  (1 << 6) /* RESOLV_PATH for *_at functions */
#define RESOLV_PATH_AT2 (1 << 7) /* RESOLV_PATH2 for *_at functions */
#define OPEN_MODE       (1 << 8) /* Check the write mode of open flags */
#define FAKE_ID         (1 << 9) /* Fake builder identity as root */
#define NET_CALL        (1 << 10) /* System call depends on network allowed flag */
#define DENY_ALL        (1 << 11) /* Insecure system call that needs to be denied */

/* System call dispatch table */
static struct syscall_def {
    int no;
    const char *name;
    unsigned int flags;
} system_calls[] = {
    {__NR_chmod,        "chmod",        CHECK_PATH | RESOLV_PATH},
    {__NR_chown,        "chown",        CHECK_PATH | RESOLV_PATH},
    {__NR_open,         "open",         CHECK_PATH | OPEN_MODE | RESOLV_PATH},
    {__NR_creat,        "creat",        CHECK_PATH | RESOLV_PATH},
    {__NR_lchown,       "lchown",       CHECK_PATH},
    {__NR_link,         "link",         0},
    {__NR_mkdir,        "mkdir",        CHECK_PATH | RESOLV_PATH},
    {__NR_mknod,        "mknod",        CHECK_PATH | RESOLV_PATH},
    {__NR_access,       "access",       0},
    {__NR_rename,       "rename",       CHECK_PATH | RESOLV_PATH},
    {__NR_rmdir,        "rmdir",        CHECK_PATH | RESOLV_PATH},
    {__NR_symlink,      "symlink",      0},
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
    {__NR_ptrace,       "ptrace",       0},
    {0,                 NULL,           0}
};

int syscall_check(context_t *ctx, struct tchild *child, int syscall) {
    int i, issymlink;
    char *rpath;
    unsigned int sflags;
    const char *sname;
    char pathname[PATH_MAX];
    int flags;

    for (i = 0; system_calls[i].name; i++) {
        if (system_calls[i].no == syscall)
            goto found;
    }
    return 0;
found:
    sflags = system_calls[i].flags;
    sname = system_calls[i].name;
    lg(LOG_VERBOSE, "syscall.syscall_check.essential",
            "Child %i called essential system call %s()", child->pid, sname);

    if (sflags & CHECK_PATH) {
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
                    return -1;
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
                return -1;
            }
        }

        if (sflags & OPEN_MODE) {
#if defined(I386)
            flags = ptrace(PTRACE_PEEKUSER, child->pid, 4 * ECX, NULL);
#elif defined(X86_64)
            flags = ptrace(PTRACE_PEEKUSER, child->pid, 8 * RSI, NULL);
#endif
            if (!(flags & O_WRONLY || flags & O_RDWR)) {
                if (issymlink) {
                    /* Change the pathname argument with the resolved path to
                     * prevent symlink races.
                     */
                    ptrace_set_string(child->pid, 1, rpath, PATH_MAX);
                }
                return 0;
            }
        }

        if (pathlist_check(&(ctx->predict_prefixes), rpath)) {
            /* Predict mode change path parameter to /dev/null */
            lg(LOG_VERBOSE, "syscall.syscall_check.predict",
                    "\"%s\" is under a predict path, changing path argument to /dev/null", rpath);
            ptrace_set_string(child->pid, 1, "/dev/null", 10);
            free(rpath);
            return 0;
        }
        if (!pathlist_check(&(ctx->write_prefixes), rpath)) {
            if (sflags & OPEN_MODE)
                access_error(child->pid, "%s(\"%s\", O_WRONLY/O_RDWR, ...)", sname, pathname);
            else
                access_error(child->pid, "%s(\"%s\", ...)", sname, pathname);
            free(rpath);
            return -1;
        }

        if (issymlink) {
            /* Change the pathname argument with the resolved path to
            * prevent symlink races.
            */
            ptrace_set_string(child->pid, 1, rpath, PATH_MAX);
        }
        free(rpath);
    }
    return 0;
}

int syscall_handle(context_t *ctx, struct tchild *child) {
    int syscall, ret;

    syscall = ptrace_get_syscall(child->pid);

    if (!child->in_syscall) { /* Entering syscall */
        lg(LOG_DEBUG, "syscall.syscall_handle.syscall_enter", "Child %i is entering system call number %d",
                child->pid, syscall);
        ret = syscall_check(ctx, child, syscall);
        if (0 != ret) {
            /* Access violation, prevent the system call */
            child->error_code = ret;
            child->orig_syscall = syscall;
            ptrace_set_syscall(child->pid, 0xbadca11);
        }
        child->in_syscall = 1;
    }
    else { /* Exiting syscall */
        lg(LOG_DEBUG, "syscall.syscall_handle.syscall_exit", "Child %i is exiting system call number %d",
                child->pid, syscall);
        if (0xbadca11 == syscall) {
            /* Restore real call number and return our error code */
            ptrace_set_syscall(child->pid, child->orig_syscall);
#if defined(I386)
            if (0 != ptrace(PTRACE_POKEUSER, child->pid, ADDR_MUL * EAX, child->error_code)) {
#elif defined(X86_64)
            if (0 != ptrace(PTRACE_POKEUSER, child->pid, ADDR_MUL * RAX, child->error_code)) {
#endif
                lg(LOG_ERROR, "syscall.syscall_handle.fail_pokeuser",
                        "Failed to set error code to %d: %s", child->error_code, strerror(errno));
                return -1;
            }
        }
        child->in_syscall = 0;
    }
    return 0;
}
