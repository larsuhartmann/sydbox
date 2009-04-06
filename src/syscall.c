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

#define _ATFILE_SOURCE // AT_FDCWD

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <sys/stat.h>

#include <glib.h>

#include "sydbox-config.h"

#include "getcwd.h"
#include "log.h"
#include "path.h"
#include "util.h"
#include "proc.h"
#include "trace.h"
#include "syscall.h"
#include "children.h"
#include "wrappers.h"

// System call dispatch flags
#define RETURNS_FD              (1 << 0) // The function returns a file descriptor
#define OPEN_MODE               (1 << 1) // Check the mode argument of open()
#define OPEN_MODE_AT            (1 << 2) // Check the mode argument of openat()
#define ACCESS_MODE             (1 << 3) // Check the mode argument of access()
#define ACCESS_MODE_AT          (1 << 4) // Check the mode argument of faccessat()
#define CHECK_PATH              (1 << 5) // First argument should be a valid path
#define CHECK_PATH2             (1 << 6) // Second argument should be a valid path
#define CHECK_PATH_AT           (1 << 7) // CHECK_PATH for at suffixed functions
#define CHECK_PATH_AT2          (1 << 8) // CHECK_PATH2 for at suffixed functions
#define DONT_RESOLV             (1 << 9) // Don't resolve symlinks
#define IF_AT_SYMLINK_FOLLOW4   (1 << 10) // Resolving path depends on AT_SYMLINK_FOLLOW (4th argument)
#define IF_AT_SYMLINK_NOFOLLOW3 (1 << 11) // Resolving path depends on AT_SYMLINK_NOFOLLOW (3th argument)
#define IF_AT_SYMLINK_NOFOLLOW4 (1 << 12) // Resolving path depends on AT_SYMLINK_NOFOLLOW (4th argument)
#define IF_AT_REMOVEDIR2        (1 << 13) // Resolving path depends on AT_REMOVEDIR (2nd argument)
#define CAN_CREAT               (1 << 14) // The system call can create the first path if it doesn't exist
#define CAN_CREAT2              (1 << 15) // The system call can create the second path if it doesn't exist
#define CAN_CREAT_AT            (1 << 16) // CAN_CREAT for at suffixed functions
#define CAN_CREAT_AT2           (1 << 17) // CAN_CREAT_AT2 for at suffixed functions
#define MAGIC_OPEN              (1 << 18) // Check if the open() call is magic
#define MAGIC_STAT              (1 << 19) // Check if the stat() call is magic
#define NET_CALL                (1 << 20) // Allowing the system call depends on the net flag

static const struct syscall_name {
    int no;
    const char *name;
} sysnames[] = {
#include "syscallent.h"
{-1,    NULL}
};

// System call dispatch table
static const struct syscall_def syscalls[] = {
    {__NR_chmod,        CHECK_PATH},
    {__NR_chown,        CHECK_PATH},
#if defined(__NR_chown32)
    {__NR_chown32,      CHECK_PATH},
#endif
    {__NR_open,         CHECK_PATH | RETURNS_FD | OPEN_MODE | MAGIC_OPEN},
    {__NR_creat,        CHECK_PATH | CAN_CREAT | RETURNS_FD},
    {__NR_stat,         MAGIC_STAT},
#if defined(__NR_stat64)
    {__NR_stat64,       MAGIC_STAT},
#endif
    {__NR_lchown,       CHECK_PATH | DONT_RESOLV},
#if defined(__NR_lchown32)
    {__NR_lchown32,     CHECK_PATH | DONT_RESOLV},
#endif
    {__NR_link,         CHECK_PATH | CHECK_PATH2 | CAN_CREAT2 | DONT_RESOLV},
    {__NR_mkdir,        CHECK_PATH | CAN_CREAT},
    {__NR_mknod,        CHECK_PATH | CAN_CREAT},
    {__NR_access,       CHECK_PATH | ACCESS_MODE},
    {__NR_rename,       CHECK_PATH | CHECK_PATH2 | CAN_CREAT2},
    {__NR_rmdir,        CHECK_PATH},
    {__NR_symlink,      CHECK_PATH2 | CAN_CREAT2 | DONT_RESOLV},
    {__NR_truncate,     CHECK_PATH},
#if defined(__NR_truncate64)
    {__NR_truncate64,   CHECK_PATH},
#endif
    {__NR_mount,        CHECK_PATH2},
#if defined(__NR_umount)
    {__NR_umount,       CHECK_PATH},
#endif
    {__NR_umount2,      CHECK_PATH},
    {__NR_utime,        CHECK_PATH},
    {__NR_unlink,       CHECK_PATH | DONT_RESOLV},
    {__NR_openat,       CHECK_PATH_AT | OPEN_MODE_AT | RETURNS_FD},
    {__NR_mkdirat,      CHECK_PATH_AT | CAN_CREAT_AT},
    {__NR_mknodat,      CHECK_PATH_AT | CAN_CREAT_AT},
    {__NR_fchownat,     CHECK_PATH_AT | IF_AT_SYMLINK_NOFOLLOW4},
    {__NR_unlinkat,     CHECK_PATH_AT | IF_AT_REMOVEDIR2},
    {__NR_renameat,     CHECK_PATH_AT | CHECK_PATH_AT2 | CAN_CREAT_AT2},
    {__NR_linkat,       CHECK_PATH_AT | CHECK_PATH_AT2 | CAN_CREAT_AT2 | IF_AT_SYMLINK_FOLLOW4},
    {__NR_symlinkat,    CHECK_PATH_AT | CHECK_PATH_AT2 | CAN_CREAT_AT2 | DONT_RESOLV},
    {__NR_fchmodat,     CHECK_PATH_AT | IF_AT_SYMLINK_NOFOLLOW3},
    {__NR_faccessat,    CHECK_PATH_AT | ACCESS_MODE_AT},
#if defined(__NR_socketcall)
    {__NR_socketcall,   NET_CALL},
#elif defined(__NR_socket)
    {__NR_socket,       NET_CALL},
#else
#error unsupported architecture
#endif
    {-1,                 0}
};

enum res_flag {
    RF_ERROR, // Error occured while checking mode
    RF_WRITE, // Write flags set in mode (O_WR, O_WRONLY or O_RDWR)
    RF_NOWRITE, // No write flags in mode
    RF_CREAT, // O_CREAT is in mode
};

static inline const char *syscall_get_name(int no) {
    for (int i = 0; sysnames[i].name != NULL; i++) {
        if (sysnames[i].no == no)
            return sysnames[i].name;
    }
    return NULL;
}

static inline bool syscall_can_creat(int arg, int flags) {
    if (0 == arg && flags & CAN_CREAT)
        return true;
    else if (1 == arg && flags & CAN_CREAT2)
        return true;
    else if (2 == arg && flags & CAN_CREAT_AT)
        return true;
    else if (3 == arg && flags & CAN_CREAT_AT2)
        return true;
    else
        return false;
}

static inline bool syscall_needs_flagcheck(int flags) {
    if (flags & OPEN_MODE)
        return true;
    else if (flags & OPEN_MODE_AT)
        return true;
    else if (flags & ACCESS_MODE)
        return true;
    else if (flags & ACCESS_MODE_AT)
        return true;
    else
        return false;
}

static enum res_flag syscall_has_flagwrite(pid_t pid, unsigned int sflags) {
    int ret, save_errno;
    long flags;
    if (sflags & OPEN_MODE || sflags & ACCESS_MODE) {
        ret = trace_get_arg(pid, 1, &flags);
    } else if (sflags & OPEN_MODE_AT || sflags & ACCESS_MODE_AT) {
        ret = trace_get_arg(pid, 2, &flags);
    } else {
        g_printerr ("bug in syscall_has_flagwrite() call");
        exit (-1);
    }

    if (0 > ret) {
        save_errno = errno;
        g_critical ("failed to get flags argument: %s", strerror(errno));
        errno = save_errno;
        return RF_ERROR;
    }

    if (sflags & OPEN_MODE || sflags & OPEN_MODE_AT) {
        if (flags & O_CREAT)
            return RF_CREAT;
        else if (flags & O_WRONLY || flags & O_RDWR)
            return RF_WRITE;
        else
            return RF_NOWRITE;
    } else if (sflags & ACCESS_MODE || sflags & ACCESS_MODE_AT) {
        if (flags & W_OK)
            return RF_WRITE;
        else
            return RF_NOWRITE;
    } else {
        g_printerr ("bug in syscall_has_flagwrite() call");
        exit (-1);
    }
}

static char *syscall_get_rpath(context_t *ctx, struct tchild *child, unsigned int flags,
        char *path, bool has_creat, unsigned int npath) {
    long len;
    char *pathc, *path_sanitized, *rpath;
    bool resolve;

    if ('/' != path[0] && 0 != strncmp(ctx->cwd, child->cwd, strlen(ctx->cwd) + 1)) {
        char *newcwd;
        if (0 == strncmp(ctx->cwd, child->cwd, strlen(ctx->cwd))) {
            /* child->cwd begins with ctx->cwd, call chdir using relative path
             * instead of absolute path to make sure no errors regarding
             * permissions happen.
             */
            newcwd = child->cwd + strlen(ctx->cwd) + 1;
        }
        else
            newcwd = child->cwd;

        if (0 > echdir(newcwd)) {
            g_debug ("failed to change current working directory to `%s': %s", newcwd, strerror(errno));
            g_debug ("adding current working directory to `%s' instead", path);
            len = strlen(child->cwd) + strlen(path) + 2;
            pathc = g_malloc (len * sizeof(char));
            snprintf(pathc, len, "%s/%s", child->cwd, path);
            path_sanitized = remove_slash(pathc);
            g_free (pathc);
        }
        else {
            g_free (ctx->cwd);
            ctx->cwd = g_strdup (child->cwd);
            path_sanitized = remove_slash(path);
        }
    }
    else
        path_sanitized = remove_slash(path);

    if (flags & DONT_RESOLV)
        resolve = false;
    else if (flags & IF_AT_SYMLINK_FOLLOW4) {
        long symflags;
        if (0 > trace_get_arg(child->pid, 4, &symflags)) {
            g_free (path_sanitized);
            return NULL;
        }
        resolve = symflags & AT_SYMLINK_FOLLOW ? true : false;
    }
    else if (flags & IF_AT_SYMLINK_NOFOLLOW3 || flags & IF_AT_SYMLINK_NOFOLLOW4) {
        long symflags;
        int arg = flags & IF_AT_SYMLINK_NOFOLLOW3 ? 3 : 4;
        if (0 > trace_get_arg(child->pid, arg, &symflags)) {
            g_free (path_sanitized);
            return NULL;
        }
        resolve = symflags & AT_SYMLINK_NOFOLLOW ? false : true;
    }
    else if (flags & IF_AT_REMOVEDIR2) {
        long rmflags;
        if (0 > trace_get_arg(child->pid, 2, &rmflags)) {
            g_free (path_sanitized);
            return NULL;
        }
        resolve = rmflags & AT_REMOVEDIR ? true : false;
    }
    else
        resolve = true;

    if (has_creat || syscall_can_creat(npath, flags)) {
        g_debug ("mode is CAN_ALL_BUT_LAST resolve is %s", resolve ? "true" : "false");
        rpath = canonicalize_filename_mode(path_sanitized, CAN_ALL_BUT_LAST, resolve, child->cwd);
    }
    else {
        g_debug ("mode is CAN_EXISTING resolve is %s", resolve ? "true" : "false");
        rpath = canonicalize_filename_mode(path_sanitized, CAN_EXISTING, resolve, child->cwd);
    }

    g_free (path_sanitized);
    return rpath;
}

static char *syscall_get_dirname(context_t *ctx, struct tchild *child, unsigned int npath) {
    int save_errno;
    long dfd;

    assert(0 == npath || 2 == npath);
    if (0 > trace_get_arg(child->pid, npath, &dfd)) {
        save_errno = errno;
        g_warning ("failed to get dirfd: %s", strerror(errno));
        errno = save_errno;
        return NULL;
    }

    if (AT_FDCWD == dfd)
        return child->cwd;
    else
        return pgetdir(ctx, child->pid, dfd);
}

static enum res_syscall syscall_check_path(struct tchild *child, const struct syscall_def *sdef,
        int paranoid, const char *path, int npath) {
    g_debug ("checking \"%s\" for write access", path);
    int allow_write = pathlist_check(child->sandbox->write_prefixes, path);
    g_debug ("checking \"%s\" for predict access", path);
    int allow_predict = pathlist_check(child->sandbox->predict_prefixes, path);

    if (!allow_write && !allow_predict) {
        const char *sname;
        char *reason = g_malloc ((strlen(path) + 256) * sizeof(char));
        child->retval = -EPERM;
        if (0 == npath)
            strcpy(reason, "%s(\"%s\", ");
        else if (1 == npath)
            strcpy(reason, "%s(?, \"%s\", ");
        else if (2 == npath)
            strcpy(reason, "%s(?, ?, \"%s\", ");
        else if (3 == npath)
            strcpy(reason, "%s(?, ?, ?, \"%s\", ");

        if (sdef->flags & ACCESS_MODE)
            strcat(reason, "O_WR)");
        else if (sdef->flags & OPEN_MODE || sdef->flags & OPEN_MODE_AT)
            strcat(reason, "O_WRONLY/O_RDWR)");
        else
            strcat(reason, "...)");
        sname = syscall_get_name(sdef->no);
        access_error(child->pid, reason, sname, path);
        g_free (reason);
        return RS_DENY;
    }
    else if (!allow_write && allow_predict) {
        if (sdef->flags & RETURNS_FD) {
            g_debug ("system call returns fd and its argument is under a predict path");
            g_debug ("changing the path argument to /dev/null");
            if (0 > trace_set_string(child->pid, npath, "/dev/null", 10)) {
                g_printerr ("failed to set string: %s", g_strerror (errno));
                exit (-1);
            }
            return RS_ALLOW;
        }
        else {
            child->retval = 0;
            return RS_DENY;
        }
    }

    if (paranoid && !(sdef->flags & DONT_RESOLV)) {
        /* Change the path argument with the resolved path to
         * prevent symlink races.
         */
        g_debug ("paranoia! System call has DONT_RESOLV unset, substituting path with resolved path");
        if (0 > trace_set_string(child->pid, npath, path, strlen(path) + 1)) {
            int save_errno = errno;
            g_critical ("failed to set string: %s", strerror(errno));
            errno = save_errno;
            return RS_ERROR;
        }
    }
    return RS_ALLOW;
}

static enum res_syscall syscall_check_magic_open(struct tchild *child, const char *path) {
    int ismagic = 0, save_errno;
    char *rpath_sanitized;
    const char *rpath;

    g_debug ("checking if open(\"%s\", ...) is magic", path);
    if (path_magic_on(path)) {
        ismagic = 1;
        child->sandbox->on = 1;
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "sandbox status of child %i is now on", child->pid);
    }
    else if (path_magic_off(path)) {
        ismagic = 1;
        child->sandbox->on = 0;
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "sandbox status of child %i is now off", child->pid);
    }
    else if (path_magic_toggle(path)) {
        ismagic = 1;
        child->sandbox->on = !(child->sandbox->on);
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "sandbox status of child %i is now %s", child->pid, child->sandbox->on ? "on" : "off");
    }
    else if (path_magic_lock(path)) {
        ismagic = 1;
        child->sandbox->lock = LOCK_SET;
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "access to magic commands is now denied for child %i", child->pid);
    }
    else if (path_magic_exec_lock(path)) {
        ismagic = 1;
        child->sandbox->lock = LOCK_PENDING;
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "access to magic commands will be denied on execve() for child %i", child->pid);
    }
    else if (path_magic_write(path)) {
        ismagic = 1;
        rpath = path + CMD_WRITE_LEN;
        pathnode_new(&(child->sandbox->write_prefixes), rpath, 1);
        g_message ("approved addwrite(\"%s\") for child %i", rpath, child->pid);
    }
    else if (path_magic_predict(path)) {
        ismagic = 1;
        rpath = path + CMD_PREDICT_LEN;
        pathnode_new(&(child->sandbox->predict_prefixes), rpath, 1);
        g_message ("approved addpredict(\"%s\") for child %i", rpath, child->pid);
    }
    else if (path_magic_rmwrite(path)) {
        ismagic = 1;
        rpath = path + CMD_RMWRITE_LEN;
        rpath_sanitized = remove_slash(rpath);
        if (NULL != child->sandbox->write_prefixes)
            pathnode_delete(&(child->sandbox->write_prefixes), rpath_sanitized);
        g_message ("approved rmwrite(\"%s\") for child %i", rpath_sanitized, child->pid);
        g_free (rpath_sanitized);
    }
    else if (path_magic_rmpredict(path)) {
        ismagic = 1;
        rpath = path + CMD_RMPREDICT_LEN;
        rpath_sanitized = remove_slash(rpath);
        if (NULL != child->sandbox->predict_prefixes)
            pathnode_delete(&(child->sandbox->predict_prefixes), rpath_sanitized);
        g_message ("approved rmpredict(\"%s\") for child %i", rpath_sanitized, child->pid);
        g_free (rpath_sanitized);
    }

    if (ismagic) {
        g_debug ("changing path to /dev/null");
        if (0 > trace_set_string(child->pid, 0, "/dev/null", 10)) {
            save_errno = errno;
            g_critical ("failed to set string to /dev/null: %s", strerror(errno));
            errno = save_errno;
            return RS_ERROR;
        }
        return RS_ALLOW;
    }

    g_debug ("open(\"%s\", ...) not magic", path);
    return RS_NONMAGIC;
}

static enum res_syscall syscall_check_magic_stat(struct tchild *child) {
    int save_errno;
    char *path;

    path = trace_get_string(child->pid, 0);
    if (NULL == path) {
        save_errno = errno;
        g_critical ("failed to get string from argument 0: %s", strerror(errno));
        errno = save_errno;
        return RS_ERROR;
    }
    g_debug ("checking if stat(\"%s\") is magic", path);
    if (path_magic_dir(path)) {
        g_debug ("stat(\"%s\") is magic, faking stat buffer", path);
        if (0 > trace_fake_stat(child->pid)) {
            save_errno = errno;
            g_critical ("failed to fake stat: %s", strerror(errno));
            errno = save_errno;
            return RS_ERROR;
        }
        g_free (path);
        child->retval = 0;
        return RS_DENY;
    }
    else {
        g_debug ("stat(\"%s\") is not magic", path);
        g_free (path);
        return RS_NONMAGIC;
    }
}

enum res_syscall syscall_check(context_t *ctx, struct tchild *child, int sno) {
    unsigned int i, ret, save_errno;
    char *path = NULL, *pathfirst = NULL, *rpath = NULL, *oldcwd = NULL;
    const char *sname;
    const struct syscall_def *sdef;

    for (i = 0; syscalls[i].no != -1; i++) {
        if (syscalls[i].no == sno)
            goto found;
    }
    return RS_ALLOW;
found:
    sdef = &(syscalls[i]);
#if 0
    if (LOG_DEBUG <= log_level)
        sname = syscall_get_name(sdef->no);
    else
#endif
#warning "lookup name for verbosity >= 3"
        sname = NULL;

    g_debug ("child %i called essential system call %s()", child->pid, sname);

    /* Check write flags like O_WRONLY / O_RDWR
     * We do this before handling magic calls because all magic commands that
     * opens a file must write to it anyway.
     */
    bool has_creat = false;
    if (syscall_needs_flagcheck(sdef->flags)) {
        switch (syscall_has_flagwrite(child->pid, sdef->flags)) {
            case RF_ERROR:
                return RS_ERROR;
            case RF_NOWRITE:
                return RS_ALLOW;
            case RF_CREAT:
                has_creat = true;
                break;
            case RF_WRITE:
            default:
                break;
        }
    }

    // Handle magic calls
    if (LOCK_SET != child->sandbox->lock) {
        if (sdef->flags & MAGIC_OPEN) {
            pathfirst = trace_get_string(child->pid, 0);
            if (NULL == pathfirst) {
                save_errno = errno;
                g_critical ("failed to get string from argument 0: %s", strerror(errno));
                errno = save_errno;
                return RS_ERROR;
            }
            ret = syscall_check_magic_open(child, pathfirst);
            if (RS_NONMAGIC != ret) {
                g_free (pathfirst);
                return ret;
            }
            /* Special case, to avoid getting the path argument of open()
             * twice, one for this one and one for CHECK_PATH, we don't free it
             * here.
             * g_free(pathfirst);
             */
        }
        else if (sdef->flags & MAGIC_STAT) {
            ret = syscall_check_magic_stat(child);
            if (RS_NONMAGIC != ret)
                return ret;
        }
    }

    // If sandbox is disabled allow any system call
    if (!child->sandbox->on)
        return RS_ALLOW;

    // Check paths
    if (sdef->flags & CHECK_PATH) {
        g_debug ("system call %s() has CHECK_PATH set, checking", sname);
        if (NULL == pathfirst) {
            pathfirst = trace_get_string(child->pid, 0);
            if (NULL == pathfirst) {
                save_errno = errno;
                g_critical ("failed to get string from argument 0: %s", strerror(errno));
                errno = save_errno;
                return RS_ERROR;
            }
        }
        rpath = syscall_get_rpath(ctx, child, sdef->flags, pathfirst, has_creat, 0);
        if (NULL == rpath) {
            child->retval = -errno;
            g_debug ("canonicalize_filename_mode() failed for `%s': %s", pathfirst, strerror(errno));
            g_free (pathfirst);
            return RS_DENY;
        }
        g_free (pathfirst);
        ret = syscall_check_path(child, sdef, sydbox_config_get_paranoid_mode_enabled (), rpath, 0);
        g_free (rpath);
        /* Return here only if access is denied because some syscalls have
         * both CHECK_PATH and CHECK_PATH2 set.
         */
        if (RS_ALLOW != ret)
            return ret;
    }
    if (sdef->flags & CHECK_PATH2) {
        g_debug ("system call %s() has CHECK_PATH2 set, checking", sname);
        path = trace_get_string(child->pid, 1);
        if (NULL == path) {
            save_errno = errno;
            g_critical ("Failed to get string from argument 1: %s", strerror(errno));
            errno = save_errno;
            return RS_ERROR;
        }
        rpath = syscall_get_rpath(ctx, child, sdef->flags, path, has_creat, 1);
        if (NULL == rpath) {
            child->retval = -errno;
            g_debug ("canonicalize_filename_mode() failed for `%s': %s", path, strerror(errno));
            g_free (path);
            return RS_DENY;
        }
        g_free (path);
        ret = syscall_check_path(child, sdef, sydbox_config_get_paranoid_mode_enabled (), rpath, 1);
        g_free (rpath);
        return ret;
    }
    if (sdef->flags & CHECK_PATH_AT) {
        g_debug ("system call %s() has CHECK_PATH_AT set, checking", sname);
        path = trace_get_string(child->pid, 1);
        if (NULL == path) {
            save_errno = errno;
            g_critical ("Failed to get string from argument 1: %s", strerror(errno));
            errno = save_errno;
            return RS_ERROR;
        }
        if ('/' != path[0]) {
            char *dname = syscall_get_dirname(ctx, child, 0);
            if (NULL == dname) {
                child->retval = -errno;
                g_debug ("syscall_get_dirname() failed: %s", strerror(errno));
                g_debug ("denying access to system call %s", sname);
                return RS_DENY;
            }
            else if (child->cwd != dname) {
                /* To make at suffixed functions work when called with long
                 * paths as arguments, we change directory to dirfd here
                 * temporarily.
                 */
                oldcwd = child->cwd;
                child->cwd = dname;
            }
        }
        rpath = syscall_get_rpath(ctx, child, sdef->flags, path, has_creat, 1);
        if (NULL == rpath) {
            child->retval = -errno;
            g_debug ("canonicalize_filename_mode() failed for `%s': %s", path, strerror(errno));
            if (NULL != oldcwd) {
                child->cwd = oldcwd;
                oldcwd = NULL;
            }
            g_free (path);
            return RS_DENY;
        }
        if (NULL != oldcwd) {
            child->cwd = oldcwd;
            oldcwd = NULL;
        }
        g_free (path);
        ret = syscall_check_path(child, sdef, sydbox_config_get_paranoid_mode_enabled (), rpath, 1);
        g_free (rpath);
        /* Return here only if access is denied because some syscalls have
         * both CHECK_PATH_AT and CHECK_PATH_AT2 set.
         */
        if (RS_ALLOW != ret)
            return ret;
    }
    if (sdef->flags & CHECK_PATH_AT2) {
        g_debug ("system call %s() has CHECK_PATH_AT2 set, checking", sname);
        path = trace_get_string(child->pid, 3);
        if (NULL == path) {
            save_errno = errno;
            g_critical ("failed to get string from argument 3: %s", strerror(errno));
            errno = save_errno;
            return RS_ERROR;
        }
        if ('/' != path[0]) {
            char *dname = syscall_get_dirname(ctx, child, 2);
            if (NULL == dname) {
                child->retval = -errno;
                g_debug ("syscall_get_dirname() failed: %s", strerror(errno));
                g_debug ("denying access to system call %s", sname);
                return RS_DENY;
            }
            else if (child->cwd != dname) { // Not AT_FDCWD
                /* To make at suffixed functions work when called with long
                 * paths as arguments, we change directory to dirfd here
                 * temporarily.
                 */
                oldcwd = child->cwd;
                child->cwd = dname;
            }
        }
        rpath = syscall_get_rpath(ctx, child, sdef->flags, path, has_creat, 3);
        if (NULL == rpath) {
            child->retval = -errno;
            g_debug ("canonicalize_filename_mode() failed for `%s': %s", path, strerror(errno));
            if (NULL != oldcwd) {
                child->cwd = oldcwd;
                oldcwd = NULL;
            }
            g_free (path);
            return RS_DENY;
        }
        if (NULL != oldcwd) {
            child->cwd = oldcwd;
            oldcwd = NULL;
        }
        g_free (path);
        ret = syscall_check_path(child, sdef, sydbox_config_get_paranoid_mode_enabled (), rpath, 3);
        g_free (rpath);
        return ret;
    }
    if (sdef->flags & NET_CALL && !(child->sandbox->net)) {
#if defined(__NR_socketcall)
        access_error(child->pid, "socketcall()");
#elif defined(__NR_socket)
        access_error(child->pid, "socket()");
#endif
        child->retval = -EACCES;
        return RS_DENY;
    }
    return RS_ALLOW;
}

int syscall_handle(context_t *ctx, struct tchild *child) {
    int ret;
    long sno;
    const char *sname;

    if (0 > trace_get_syscall(child->pid, &sno)) {
        if (errno != ESRCH) {
            g_printerr ("failed to get syscall: %s", g_strerror (errno));
            exit (-1);
        }
        return handle_esrch(ctx, child);
    }
#if 0
    if (LOG_DEBUG <= log_level)
        sname = (0xbadca11 == sno) ? syscall_get_name(child->sno) : syscall_get_name(sno);
    else
#endif
#warning "lookup syscall name for verbosity >= 3"
        sname = NULL;

    if (!(child->flags & TCHILD_INSYSCALL)) { // Entering syscall
        g_log (G_LOG_DOMAIN, LOG_LEVEL_DEBUG_TRACE, "child %i is entering system call %s()", child->pid, sname);
        if (__NR_execve == sno && LOCK_PENDING == child->sandbox->lock) {
            g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "access to magic commands is now denied for child %i", child->pid);
            child->sandbox->lock = LOCK_SET;
        }
        ret = syscall_check(ctx, child, sno);
        switch (ret) {
            case RS_DENY:
                g_debug ("denying access to system call %s()", sname);
                child->sno = sno;
                if (0 > trace_set_syscall(child->pid, 0xbadca11)) {
                    if (errno != ESRCH) {
                        g_printerr ("failed to set syscall: %s", g_strerror (errno));
                        exit (-1);
                    }
                    return handle_esrch(ctx, child);
                }
                break;
            case RS_ALLOW:
                g_log (G_LOG_DOMAIN, LOG_LEVEL_DEBUG_TRACE, "allowing access to system call %s()", sname);
                break;
            case RS_ERROR:
            default:
                if (ESRCH == errno)
                    return handle_esrch(ctx, child);
                else {
                    if (NULL == sname)
                        sname = syscall_get_name(sno);
                    g_printerr ("Error while checking system call %s() for access: %s", sname, g_strerror (errno));
                    exit (-1);
                }
                break;
        }
        child->flags ^= TCHILD_INSYSCALL;
    }
    else { // Exiting syscall
        g_log (G_LOG_DOMAIN, LOG_LEVEL_DEBUG_TRACE, "child %i is exiting system call %s()", child->pid, sname);
        if (0xbadca11 == sno) {
            g_debug ("restoring real call number for denied system call %s()", sname);
            // Restore real call number and return our error code
            if (0 > trace_set_syscall(child->pid, child->sno)) {
                if (errno != ESRCH) {
                    g_printerr ("failed to restore syscall: %s", g_strerror (errno));
                    exit (-1);
                }
                return handle_esrch(ctx, child);
            }
            if (0 > trace_set_return(child->pid, child->retval)) {
                if (errno != ESRCH) {
                    g_printerr ("failed to set return code: %s", g_strerror (errno));
                    exit (-1);
                }
                return handle_esrch(ctx, child);
            }
        }
        else if (__NR_chdir == sno || __NR_fchdir == sno) {
            long retval;
            if (0 > trace_get_return(child->pid, &retval)) {
                if (errno != ESRCH) {
                    g_printerr ("failed to get return code: %s", g_strerror (errno));
                    exit (-1);
                }
                return handle_esrch(ctx, child);
            }
            if (0 == retval) {
                // Child has successfully changed directory
                char *newcwd = pgetcwd(ctx, child->pid);
                if (NULL == newcwd) {
                    retval = -errno;
                    g_debug ("pgetcwd() failed: %s", strerror(errno));
                    if (0 > trace_set_return(child->pid, retval)) {
                        if (errno != ESRCH) {
                            g_printerr ("Failed to set return code: %s", g_strerror (errno));
                            exit (-1);
                        }
                        return handle_esrch(ctx, child);
                    }
                }
                else {
                    if (NULL != child->cwd)
                        g_free (child->cwd);
                    child->cwd = newcwd;
                    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "child %i has changed directory to '%s'", child->pid, child->cwd);
                }
            }
            else
                g_debug ("child %i failed to change directory: %s", child->pid, strerror(-retval));
        }
        child->flags ^= TCHILD_INSYSCALL;
    }
    return 0;
}
