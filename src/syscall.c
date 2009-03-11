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

#include "defs.h"

// System call dispatch flags
#define RETURNS_FD      (1 << 0) // The function returns a file descriptor
#define OPEN_MODE       (1 << 1) // Check the mode argument of open()
#define OPEN_MODE_AT    (1 << 2) // Check the mode argument of openat()
#define ACCESS_MODE     (1 << 3) // Check the mode argument of access()
#define ACCESS_MODE_AT  (1 << 4) // Check the mode argument of faccessat()
#define CHECK_PATH      (1 << 5) // First argument should be a valid path
#define CHECK_PATH2     (1 << 6) // Second argument should be a valid path
#define CHECK_PATH_AT   (1 << 7) // CHECK_PATH for at suffixed functions
#define CHECK_PATH_AT2  (1 << 8) // CHECK_PATH2 for at suffixed functions
#define DONT_RESOLV     (1 << 9) // Don't resolve symlinks
#define CAN_CREAT       (1 << 10) // The system call can create the first path if it doesn't exist
#define CAN_CREAT2      (1 << 11) // The system call can create the second path if it doesn't exist
#define CAN_CREAT_AT    (1 << 12) // CAN_CREAT for at suffixed functions
#define CAN_CREAT_AT2   (1 << 13) // CAN_CREAT_AT2 for at suffixed functions
#define MAGIC_OPEN      (1 << 14) // Check if the open() call is magic
#define MAGIC_STAT      (1 << 15) // Check if the stat() call is magic
#define NET_CALL        (1 << 16) // Allowing the system call depends on the net flag

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
    {__NR_link,         CHECK_PATH | CHECK_PATH2 | CAN_CREAT2},
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
    {__NR_unlink,       CHECK_PATH},
    {__NR_openat,       CHECK_PATH_AT | OPEN_MODE_AT | RETURNS_FD},
    {__NR_mkdirat,      CHECK_PATH_AT | CAN_CREAT_AT},
    {__NR_mknodat,      CHECK_PATH_AT | CAN_CREAT_AT},
    {__NR_fchownat,     CHECK_PATH_AT},
    {__NR_unlinkat,     CHECK_PATH_AT},
    {__NR_renameat,     CHECK_PATH_AT | CHECK_PATH_AT2 | CAN_CREAT_AT2},
    {__NR_linkat,       CHECK_PATH_AT | CHECK_PATH_AT2 | CAN_CREAT_AT2},
    {__NR_symlinkat,    CHECK_PATH_AT | CHECK_PATH_AT2 | CAN_CREAT_AT2 | DONT_RESOLV},
    {__NR_fchmodat,     CHECK_PATH_AT},
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
    if (sflags & OPEN_MODE || sflags & ACCESS_MODE)
        ret = trace_get_arg(pid, 1, &flags);
    else if (sflags & OPEN_MODE_AT || sflags & ACCESS_MODE_AT)
        ret = trace_get_arg(pid, 2, &flags);
    else
        DIESOFT("Bug in syscall_has_flagwrite() call");

    if (0 > ret) {
        save_errno = errno;
        LOGE("Failed to get flags argument: %s", strerror(errno));
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
    }
    else if (sflags & ACCESS_MODE || sflags & ACCESS_MODE_AT) {
        if (flags & W_OK)
            return RF_WRITE;
        else
            return RF_NOWRITE;
    }
    else
        DIESOFT("Bug in syscall_has_flagwrite() call");
}

static char *syscall_get_rpath(struct tchild *child, unsigned int flags,
        char *path, bool has_creat, unsigned int npath) {
    long len;
    char *pathc, *rpath;
    bool free_pathc = false;

    if ('/' != path[0]) {
        // Add current working directory
        LOGD("`%s' is not an absolute path, adding cwd `%s'", path, child->cwd);
        len = strlen(child->cwd) + strlen(path) + 2;
        pathc = xmalloc(len * sizeof(char));
        snprintf(pathc, len, "%s/%s", child->cwd, path);
        free_pathc = true;
    }
    else
        pathc = path;

    if (has_creat || syscall_can_creat(npath, flags)) {
        LOGC("System call may create the file, setting mode to CAN_ALL_BUT_LAST");
        if (flags & DONT_RESOLV)
            rpath = canonicalize_filename_mode(pathc, CAN_ALL_BUT_LAST, false);
        else
            rpath = canonicalize_filename_mode(pathc, CAN_ALL_BUT_LAST, true);
    }
    else {
        LOGC("System call can't create the file, setting mode to CAN_EXISTING");
        if (flags & DONT_RESOLV)
            rpath = canonicalize_filename_mode(pathc, CAN_EXISTING, false);
        else
            rpath = canonicalize_filename_mode(pathc, CAN_EXISTING, true);
    }

    if (free_pathc)
        free(pathc);
    return rpath;
}

static char *syscall_get_pathat(pid_t pid, unsigned int npath) {
    int save_errno;
    long dirfd;
    char *buf;

    assert(1 == npath || 3 == npath);
    if (0 > trace_get_arg(pid, npath - 1, &dirfd)) {
        save_errno = errno;
        LOGE("Failed to get dirfd: %s", strerror(errno));
        errno = save_errno;
        return NULL;
    }
    buf = trace_get_string(pid, npath);
    if (NULL == buf) {
        save_errno = errno;
        LOGE("Failed to get string from argument %d: %s", npath, strerror(errno));
        errno = save_errno;
        return NULL;
    }

    if (AT_FDCWD != dirfd && '/' != buf[0]) {
        char procfd[128];
        char *dname;

        snprintf(procfd, 128, "/proc/%i/fd/%ld", pid, dirfd);
        dname = ereadlink(procfd);
        if (NULL == dname) {
            save_errno = errno;
            LOGE("readlink() failed for `%s': %s", dname, strerror(errno));
            errno = save_errno;
            return NULL;
        }

        char *destc = xstrndup(buf, strlen(buf) + 1);
        buf = xrealloc(buf, strlen(buf) + strlen(dname) + 2);
        sprintf(buf, "%s/%s", dname, destc);
        free(dname);
        free(destc);
    }
    return buf;
}

static enum res_syscall syscall_check_path(struct tchild *child, const struct syscall_def *sdef,
        int paranoid, const char *path, int npath) {
    LOGD("Checking \"%s\" for write access", path);
    int allow_write = pathlist_check(&(child->sandbox->write_prefixes), path);
    LOGD("Checking \"%s\" for predict access", path);
    int allow_predict = pathlist_check(&(child->sandbox->predict_prefixes), path);

    if (!allow_write && !allow_predict) {
        const char *sname;
        char *reason = xmalloc((strlen(path) + 256) * sizeof(char));
        child->retval = -EPERM;
        if (0 == npath)
            strcpy(reason, "%s(\"%s\", ");
        else if (1 == npath)
            strcpy(reason, "%s(?, \"%s\", ");
        else
            strcpy(reason, "%s(DIRFD, \"%s\", ");

        if (sdef->flags & ACCESS_MODE)
            strcat(reason, "O_WR)");
        else if (sdef->flags & OPEN_MODE || sdef->flags & OPEN_MODE_AT)
            strcat(reason, "O_WRONLY/O_RDWR)");
        else
            strcat(reason, "...)");
        sname = syscall_get_name(sdef->no);
        access_error(child->pid, reason, sname, path);
        free(reason);
        return RS_DENY;
    }
    else if (!allow_write && allow_predict) {
        if (sdef->flags & RETURNS_FD) {
            LOGD("System call returns fd and its argument is under a predict path");
            LOGD("Changing the path argument to /dev/null");
            if (0 > trace_set_string(child->pid, npath, "/dev/null", 10))
                DIESOFT("Failed to set string: %s", strerror(errno));
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
        LOGD("Paranoia! System call has DONT_RESOLV unset, substituting path with resolved path");
        if (0 > trace_set_string(child->pid, npath, path, strlen(path) + 1)) {
            int save_errno = errno;
            LOGE("Failed to set string: %s", strerror(errno));
            errno = save_errno;
            return RS_ERROR;
        }
    }
    return RS_ALLOW;
}

static enum res_syscall syscall_check_magic_open(struct tchild *child, const char *path) {
    int ismagic = 0, save_errno;
    const char *rpath;

    LOGD("Checking if open(\"%s\", ...) is magic", path);
    if (path_magic_on(path)) {
        ismagic = 1;
        child->sandbox->on = 1;
        LOGN("Sandbox status of child %i is now on", child->pid);
    }
    else if (path_magic_off(path)) {
        ismagic = 1;
        child->sandbox->on = 0;
        LOGN("Sandbox status of child %i is now off", child->pid);
    }
    else if (path_magic_toggle(path)) {
        ismagic = 1;
        child->sandbox->on = !(child->sandbox->on);
        LOGN("Sandbox status of child %i is now %s", child->pid, child->sandbox->on ? "on" : "off");
    }
    else if (path_magic_lock(path)) {
        ismagic = 1;
        LOGN("Access to magic commands is now denied for child %i", child->pid);
        child->sandbox->lock = LOCK_SET;
    }
    else if (path_magic_exec_lock(path)) {
        ismagic = 1;
        LOGN("Access to magic commands will be denied on execve() for child %i", child->pid);
        child->sandbox->lock = LOCK_PENDING;
    }
    else if (path_magic_write(path)) {
        ismagic = 1;
        rpath = path + CMD_WRITE_LEN;
        LOGN("Approved addwrite(\"%s\") for child %i", rpath, child->pid);
        pathnode_new(&(child->sandbox->write_prefixes), rpath, 1);
    }
    else if (path_magic_predict(path)) {
        ismagic = 1;
        rpath = path + CMD_PREDICT_LEN;
        LOGN("Approved addpredict(\"%s\") for child %i", rpath, child->pid);
        pathnode_new(&(child->sandbox->predict_prefixes), rpath, 1);
    }
    else if (path_magic_rmwrite(path)) {
        ismagic = 1;
        rpath = path + CMD_RMWRITE_LEN;
        LOGN("Approved rmwrite(\"%s\") for child %i", rpath, child->pid);
        if (NULL != child->sandbox->write_prefixes)
            pathnode_delete(&(child->sandbox->write_prefixes), rpath);
    }
    else if (path_magic_rmpredict(path)) {
        ismagic = 1;
        rpath = path + CMD_RMPREDICT_LEN;
        LOGN("Approved rmpredict(\"%s\") for child %i", rpath, child->pid);
        if (NULL != child->sandbox->predict_prefixes)
            pathnode_delete(&(child->sandbox->predict_prefixes), rpath);
    }

    if (ismagic) {
        // Change argument to /dev/null
        LOGD("Changing path to /dev/null");
        if (0 > trace_set_string(child->pid, 0, "/dev/null", 10)) {
            save_errno = errno;
            LOGE("Failed to set string to /dev/null: %s", strerror(errno));
            errno = save_errno;
            return RS_ERROR;
        }
        return RS_ALLOW;
    }

    LOGD("open(\"%s\", ...) not magic", path);
    return RS_NONMAGIC;
}

static enum res_syscall syscall_check_magic_stat(struct tchild *child) {
    int save_errno;
    char *path;

    path = trace_get_string(child->pid, 0);
    if (NULL == path) {
        save_errno = errno;
        LOGE("Failed to get string from argument 0: %s", strerror(errno));
        errno = save_errno;
        return RS_ERROR;
    }
    LOGD("Checking if stat(\"%s\") is magic", path);
    if (path_magic_dir(path)) {
        LOGD("stat(\"%s\") is magic, faking stat buffer", path);
        if (0 > trace_fake_stat(child->pid)) {
            save_errno = errno;
            LOGE("Failed to fake stat: %s", strerror(errno));
            errno = save_errno;
            return RS_ERROR;
        }
        free(path);
        child->retval = 0;
        return RS_DENY;
    }
    else {
        LOGD("stat(\"%s\") is not magic", path);
        free(path);
        return RS_NONMAGIC;
    }
}

enum res_syscall syscall_check(context_t *ctx, struct tchild *child, int sno) {
    unsigned int i, ret, save_errno;
    char *path = NULL, *pathfirst = NULL, *rpath = NULL;
    const char *sname;
    const struct syscall_def *sdef;

    for (i = 0; syscalls[i].no != -1; i++) {
        if (syscalls[i].no == sno)
            goto found;
    }
    return RS_ALLOW;
found:
    sdef = &(syscalls[i]);
    if (LOG_DEBUG <= log_level)
        sname = syscall_get_name(sdef->no);
    else
        sname = NULL;

    LOGD("Child %i called essential system call %s()", child->pid, sname);

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
                LOGE("Failed to get string from argument 0: %s", strerror(errno));
                errno = save_errno;
                return RS_ERROR;
            }
            ret = syscall_check_magic_open(child, pathfirst);
            if (RS_NONMAGIC != ret) {
                free(pathfirst);
                return ret;
            }
            /* Special case, to avoid getting the path argument of open()
             * twice, one for this one and one for CHECK_PATH, we don't free it
             * here.
             * free(pathfirst);
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
        LOGD("System call %s() has CHECK_PATH set, checking", sname);
        if (NULL == pathfirst) {
            pathfirst = trace_get_string(child->pid, 0);
            if (NULL == pathfirst) {
                save_errno = errno;
                LOGE("Failed to get string from argument 0: %s", strerror(errno));
                errno = save_errno;
                return RS_ERROR;
            }
        }
        rpath = syscall_get_rpath(child, sdef->flags, pathfirst, has_creat, 0);
        if (NULL == rpath) {
            child->retval = -errno;
            LOGD("canonicalize_filename_mode() failed for `%s': %s", pathfirst, strerror(errno));
            free(pathfirst);
            return RS_ERROR;
        }
        free(pathfirst);
        ret = syscall_check_path(child, sdef, ctx->paranoid, rpath, 0);
        free(rpath);
        /* Return here only if access is denied because some syscalls have
         * both CHECK_PATH and CHECK_PATH2 set.
         */
        if (RS_ALLOW != ret)
            return ret;
    }
    if (sdef->flags & CHECK_PATH2) {
        LOGD("System call %s() has CHECK_PATH2 set, checking", sname);
        path = trace_get_string(child->pid, 1);
        if (NULL == path) {
            save_errno = errno;
            LOGE("Failed to get string from argument 1: %s", strerror(errno));
            errno = save_errno;
            return RS_ERROR;
        }
        rpath = syscall_get_rpath(child, sdef->flags, path, has_creat, 1);
        if (NULL == rpath) {
            child->retval = -errno;
            LOGD("canonicalize_filename_mode() failed for `%s': %s", path, strerror(errno));
            free(path);
            return RS_ERROR;
        }
        free(path);
        ret = syscall_check_path(child, sdef, ctx->paranoid, rpath, 1);
        free(rpath);
        return ret;
    }
    if (sdef->flags & CHECK_PATH_AT) {
        LOGD("System call %s() has CHECK_PATH_AT set, checking", sname);
        path = syscall_get_pathat(child->pid, 1);
        if (NULL == path)
            return RS_ERROR;
        rpath = syscall_get_rpath(child, sdef->flags, path, has_creat, 1);
        if (NULL == rpath) {
            child->retval = -errno;
            LOGD("canonicalize_filename_mode() failed for `%s': %s", path, strerror(errno));
            free(path);
            return RS_ERROR;
        }
        free(path);
        ret = syscall_check_path(child, sdef, ctx->paranoid, rpath, 1);
        free(rpath);
        /* Return here only if access is denied because some syscalls have
         * both CHECK_PATH_AT and CHECK_PATH_AT2 set.
         */
        if (RS_ALLOW != ret)
            return ret;
    }
    if (sdef->flags & CHECK_PATH_AT2) {
        LOGD("System call %s() has CHECK_PATH_AT2 set, checking", sname);
        path = syscall_get_pathat(child->pid, 3);
        if (NULL == path)
            return RS_DENY;
        rpath = syscall_get_rpath(child, sdef->flags, path, has_creat, 3);
        if (NULL == rpath) {
            child->retval = -errno;
            LOGD("canonicalize_filename_mode() failed for `%s': %s", path, strerror(errno));
            free(path);
            return RS_DENY;
        }
        free(path);
        ret = syscall_check_path(child, sdef, ctx->paranoid, rpath, 3);
        free(rpath);
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
        if (ESRCH == errno)
            return handle_esrch(ctx, child);
        else
            DIESOFT("Failed to get syscall: %s", strerror(errno));
    }
    if (LOG_DEBUG <= log_level)
        sname = syscall_get_name(sno);
    else
        sname = NULL;

    if (!(child->flags & TCHILD_INSYSCALL)) { // Entering syscall
        LOGC("Child %i is entering system call %s()", child->pid, sname);
        if (__NR_execve == sno && LOCK_PENDING == child->sandbox->lock) {
            LOGN("Access to magic commands is now denied for child %i", child->pid);
            child->sandbox->lock = LOCK_SET;
        }
        ret = syscall_check(ctx, child, sno);
        switch (ret) {
            case RS_DENY:
                LOGD("Denying access to system call %s()", sname);
                child->sno = sno;
                if (0 > trace_set_syscall(child->pid, 0xbadca11)) {
                    if (ESRCH == errno)
                        return handle_esrch(ctx, child);
                    else
                        DIESOFT("Failed to set syscall: %s", strerror(errno));
                }
                break;
            case RS_ALLOW:
                LOGC("Allowing access to system call %s()", sname);
                break;
            case RS_ERROR:
            default:
                if (ESRCH == errno)
                    return handle_esrch(ctx, child);
                else {
                    if (NULL == sname)
                        sname = syscall_get_name(sno);
                    DIESOFT("Error while checking system call %s() for access: %s", sname,
                            strerror(errno));
                }
                break;
        }
        child->flags ^= TCHILD_INSYSCALL;
    }
    else { // Exiting syscall
        LOGC("Child %i is exiting system call %s()", child->pid, sname);
        if (0xbadca11 == sno) {
            LOGD("Restoring real call number for denied system call %s()", sname);
            // Restore real call number and return our error code
            if (0 > trace_set_syscall(child->pid, child->sno)) {
                if (ESRCH == errno)
                    return handle_esrch(ctx, child);
                else
                    DIESOFT("Failed to restore syscall: %s", strerror(errno));
            }
            if (0 > trace_set_return(child->pid, child->retval)) {
                if (ESRCH == errno)
                    return handle_esrch(ctx, child);
                else
                    DIESOFT("Failed to set return code: %s", strerror(errno));
            }
        }
        else if (__NR_chdir == sno || __NR_fchdir == sno) {
            long retval;
            if (0 > trace_get_return(child->pid, &retval)) {
                if (ESRCH == errno)
                    return handle_esrch(ctx, child);
                else
                    DIESOFT("Failed to get return code: %s", strerror(errno));
            }
            if (0 == retval) {
                // Child has successfully changed directory
                if (NULL != child->cwd)
                    free(child->cwd);
                child->cwd = pgetcwd(child->pid);
                if (NULL == child->cwd) {
                    retval = -errno;
                    if (0 > trace_set_return(child->pid, retval)) {
                        if (ESRCH == errno)
                            return handle_esrch(ctx, child);
                        else
                            DIESOFT("Failed to set return code: %s", strerror(errno));
                    }
                }
                LOGV("Child %i has changed directory to '%s'", child->pid, child->cwd);
            }
        }
        child->flags ^= TCHILD_INSYSCALL;
    }
    return 0;
}
