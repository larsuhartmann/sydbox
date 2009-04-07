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

#ifndef _ATFILE_SOURCE
#define _ATFILE_SOURCE
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <asm/unistd.h>

#include <glib.h>
#include <glib-object.h>

#include "log.h"
#include "path.h"
#include "proc.h"
#include "trace.h"
#include "util.h"
#include "wrappers.h"
#include "syscall_marshaller.h"
#include "syscall.h"
#include "sydbox-config.h"

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

struct syscall_def {
    int no;
    SystemCall *obj;
};
static struct syscall_def syscalls[64];

static const struct syscall_name {
    int no;
    const char *name;
} sysnames[] = {
#include "syscallent.h"
{-1,    NULL}
};

#define DEFAULT_NAME    "unknown"

enum {
    PROP_SYSTEMCALL_0,
    PROP_SYSTEMCALL_NO,
    PROP_SYSTEMCALL_FLAGS,
};

enum {
    SIGNAL_CHECK,
    LAST_SIGNAL
};

static guint systemcall_signal[LAST_SIGNAL] = { 0 };

static inline const char *syscall_get_name(int no) {
    for (int i = 0; sysnames[i].name != NULL; i++) {
        if (sysnames[i].no == no)
            return sysnames[i].name;
    }
    return NULL;
}

static void systemcall_set_property(GObject *obj,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec)
{
    SystemCall *systemcall = SYSTEMCALL(obj);

    switch (prop_id) {
        case PROP_SYSTEMCALL_NO:
            systemcall->no = g_value_get_uint(value);
            break;
        case PROP_SYSTEMCALL_FLAGS:
            systemcall->flags = g_value_get_uint(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void systemcall_get_property(GObject *obj,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec)
{
    SystemCall *systemcall;

    systemcall = SYSTEMCALL(obj);

    switch (prop_id) {
        case PROP_SYSTEMCALL_NO:
            g_value_set_uint(value, systemcall->no);
            break;
        case PROP_SYSTEMCALL_FLAGS:
            g_value_set_uint(value, systemcall->flags);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static gboolean systemcall_get_path(pid_t pid, int narg, struct checkdata *data)
{
    data->pathlist[narg] = trace_get_string(pid, narg);
    if (NULL == data->pathlist[narg]) {
        data->result = RS_ERROR;
        data->save_errno = errno;
        if (ESRCH == errno)
            g_debug("failed to grab string from argument %d: %s", narg, g_strerror(errno));
        else
            g_warning("failed to grab string from argument %d: %s", narg, g_strerror(errno));
        return FALSE;
    }
    return TRUE;
}

static gboolean systemcall_get_dirfd(SystemCall *self,
                                     context_t *ctx, struct tchild *child,
                                     int narg, struct checkdata *data)
{
    long dfd;
    if (0 > trace_get_arg(child->pid, narg, &dfd)) {
        data->result = RS_ERROR;
        data->save_errno = errno;
        if (ESRCH == errno)
            g_debug("failed to get dirfd from argument %d: %s", narg, g_strerror(errno));
        else
            g_warning("failed to get dirfd from argument %d: %s", narg, g_strerror(errno));
        return FALSE;
    }

    if (AT_FDCWD != dfd) {
        data->dirfdlist[narg] = pgetdir(ctx, child->pid, dfd);
        if (NULL == data->dirfdlist[narg]) {
            data->result = RS_DENY;
            child->retval = -errno;
            g_debug("pgetdir() failed: %s", g_strerror(errno));
            g_debug("denying access to system call %d", self->no);
            return FALSE;
        }
    }
    else
        data->dirfdlist[narg] = g_strdup(child->cwd);
    return TRUE;
}

static void systemcall_start_check(SystemCall *self, gpointer ctx_ptr,
                                   gpointer child_ptr, gpointer data_ptr)
{
    context_t *ctx = (context_t *) ctx_ptr;
    struct tchild *child = (struct tchild *) child_ptr;
    struct checkdata *data = (struct checkdata *) data_ptr;

    g_debug("starting check for system call %d, child %i", self->no, child->pid);
    if (self->flags & CHECK_PATH || self->flags & MAGIC_STAT) {
        if (!systemcall_get_path(child->pid, 0, data))
            return;
    }
    if (self->flags & CHECK_PATH2) {
        if (!systemcall_get_path(child->pid, 1, data))
            return;
    }
    if (self->flags & CHECK_PATH_AT) {
        if (!systemcall_get_dirfd(self, ctx, child, 0, data))
            return;
        if (!systemcall_get_path(child->pid, 1, data))
            return;
    }
    if (self->flags & CHECK_PATH_AT2) {
        if (!systemcall_get_dirfd(self, ctx, child, 2, data))
            return;
        if (!systemcall_get_path(child->pid, 3, data))
            return;
    }
}

static void systemcall_flags(SystemCall *self, gpointer ctx_ptr,
                             gpointer child_ptr, gpointer data_ptr)
{
    context_t *ctx = (context_t *) ctx_ptr;
    struct tchild *child = (struct tchild *) child_ptr;
    struct checkdata *data = (struct checkdata *) data_ptr;

    if (RS_ALLOW != data->result)
        return;
    else if (!(self->flags & OPEN_MODE || self->flags & OPEN_MODE_AT
                || self->flags & ACCESS_MODE || self->flags & ACCESS_MODE_AT))
        return;

    if (self->flags & OPEN_MODE || self->flags & OPEN_MODE_AT) {
        int arg = self->flags & OPEN_MODE ? 1 : 2;
        if (0 > trace_get_arg(child->pid, arg, &(data->open_flags))) {
            data->result = RS_ERROR;
            data->save_errno = errno;
            if (ESRCH == errno)
                g_debug("failed to get argument %d: %s", arg, g_strerror(errno));
            else
                g_warning("failed to get argument %d: %s", arg, g_strerror(errno));
            return;
        }
        if (!(data->open_flags & (O_CREAT | O_WRONLY | O_RDWR)))
            data->result = RS_NOWRITE;
    }
    else {
        int arg = self->flags & ACCESS_MODE ? 1 : 2;
        if (0 > trace_get_arg(child->pid, arg, &(data->access_flags))) {
            data->result = RS_ERROR;
            data->save_errno = errno;
            if (ESRCH == errno)
                g_debug("failed to get argument %d: %s", arg, g_strerror(errno));
            else
                g_warning("failed to get argument %d: %s", arg, g_strerror(errno));
            return;
        }
        if (!(data->access_flags & W_OK))
            data->result = RS_NOWRITE;
    }
}

static void systemcall_magic_open(struct tchild *child, struct checkdata *data)
{
    char *path = data->pathlist[0];
    const char *rpath;
    char *rpath_sanitized;

    g_debug ("checking if open(\"%s\", ...) is magic", path);
    if (path_magic_on(path)) {
        data->result = RS_MAGIC;
        child->sandbox->on = 1;
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "sandbox status of child %i is now on", child->pid);
    }
    else if (path_magic_off(path)) {
        data->result = RS_MAGIC;
        child->sandbox->on = 0;
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "sandbox status of child %i is now off", child->pid);
    }
    else if (path_magic_toggle(path)) {
        data->result = RS_MAGIC;
        child->sandbox->on = !(child->sandbox->on);
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "sandbox status of child %i is now %s",
                child->pid, child->sandbox->on ? "on" : "off");
    }
    else if (path_magic_lock(path)) {
        data->result = RS_MAGIC;
        child->sandbox->lock = LOCK_SET;
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "access to magic commands is now denied for child %i", child->pid);
    }
    else if (path_magic_exec_lock(path)) {
        data->result = RS_MAGIC;
        child->sandbox->lock = LOCK_PENDING;
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "access to magic commands will be denied on execve() for child %i",
                child->pid);
    }
    else if (path_magic_write(path)) {
        data->result = RS_MAGIC;
        rpath = path + CMD_WRITE_LEN;
        pathnode_new(&(child->sandbox->write_prefixes), rpath, 1);
        g_message ("approved addwrite(\"%s\") for child %i", rpath, child->pid);
    }
    else if (path_magic_predict(path)) {
        data->result = RS_MAGIC;
        rpath = path + CMD_PREDICT_LEN;
        pathnode_new(&(child->sandbox->predict_prefixes), rpath, 1);
        g_message ("approved addpredict(\"%s\") for child %i", rpath, child->pid);
    }
    else if (path_magic_rmwrite(path)) {
        data->result = RS_MAGIC;
        rpath = path + CMD_RMWRITE_LEN;
        rpath_sanitized = remove_slash(rpath);
        if (NULL != child->sandbox->write_prefixes)
            pathnode_delete(&(child->sandbox->write_prefixes), rpath_sanitized);
        g_message ("approved rmwrite(\"%s\") for child %i", rpath_sanitized, child->pid);
        g_free (rpath_sanitized);
    }
    else if (path_magic_rmpredict(path)) {
        data->result = RS_MAGIC;
        rpath = path + CMD_RMPREDICT_LEN;
        rpath_sanitized = remove_slash(rpath);
        if (NULL != child->sandbox->predict_prefixes)
            pathnode_delete(&(child->sandbox->predict_prefixes), rpath_sanitized);
        g_message ("approved rmpredict(\"%s\") for child %i", rpath_sanitized, child->pid);
        g_free (rpath_sanitized);
    }

    if (RS_MAGIC == data->result) {
        g_debug("changing path to /dev/null");
        if (0 > trace_set_string(child->pid, 0, "/dev/null", 10)) {
            data->result = RS_ERROR;
            data->save_errno = errno;
            if (ESRCH == errno)
                g_debug("failed to set string to /dev/null: %s", g_strerror(errno));
            else
                g_warning("failed to set string to /dev/null: %s", g_strerror(errno));
        }
    }
    else
        g_debug("open(\"%s\", ...) not magic", path);
}

static void systemcall_magic_stat(struct tchild *child, struct checkdata *data)
{
    char *path = data->pathlist[0];
    g_debug("checking if stat(\"%s\") is magic", path);
    if (path_magic_dir(path)) {
        g_debug("stat(\"%s\") is magic, faking stat buffer", path);
        if (0 > trace_fake_stat(child->pid)) {
            data->result = RS_ERROR;
            data->save_errno = errno;
            if (ESRCH == errno)
                g_debug("failed to fake stat buffer: %s", g_strerror(errno));
            else
                g_warning("failed to fake stat buffer: %s", g_strerror(errno));
        }
        else {
            data->result = RS_DENY;
            child->retval = 0;
        }
    }
    else
        g_debug("stat(\"%s\") is not magic", path);
}

static void systemcall_magic(SystemCall *self, gpointer ctx_ptr,
                             gpointer child_ptr, gpointer data_ptr)
{
    context_t *ctx = (context_t *) ctx_ptr;
    struct tchild *child = (struct tchild *) child_ptr;
    struct checkdata *data = (struct checkdata *) data_ptr;

    if (RS_ALLOW != data->result)
        return;
    else if (LOCK_SET == child->sandbox->lock)
        return;
    else if (__NR_open != self->no && __NR_stat != self->no)
        return;

    if (__NR_open == self->no)
        systemcall_magic_open(child, data);
    else
        systemcall_magic_stat(child, data);
}

static void systemcall_resolve(SystemCall *self, gpointer ctx_ptr,
                               gpointer child_ptr, gpointer data_ptr)
{
    context_t *ctx = (context_t *) ctx_ptr;
    struct tchild *child = (struct tchild *) child_ptr;
    struct checkdata *data = (struct checkdata *) data_ptr;

    if (RS_ALLOW != data->result)
        return;
    else if (!child->sandbox->on)
        return;

    g_debug("deciding whether we should resolve symlinks for system call %d, child %i", self->no, child->pid);
    if (self->flags & DONT_RESOLV)
        data->resolve = FALSE;
    else if (self->flags & IF_AT_SYMLINK_FOLLOW4) {
        long symflags;
        if (0 > trace_get_arg(child->pid, 4, &symflags)) {
            data->result = RS_ERROR;
            data->save_errno = errno;
            if (ESRCH == errno)
                g_debug("failed to get argument 4: %s", g_strerror(errno));
            else
                g_warning("failed to get argument 4: %s", g_strerror(errno));
            return;
        }
        data->resolve = symflags & AT_SYMLINK_FOLLOW ? TRUE : FALSE;
    }
    else if (self->flags & IF_AT_SYMLINK_NOFOLLOW3 || self->flags & IF_AT_SYMLINK_NOFOLLOW4) {
        long symflags;
        int arg = self->flags & IF_AT_SYMLINK_NOFOLLOW3 ? 3 : 4;
        if (0 > trace_get_arg(child->pid, arg, &symflags)) {
            data->result = RS_ERROR;
            data->save_errno = errno;
            if (ESRCH == errno)
                g_debug("failed to get argument %d: %s", arg, g_strerror(errno));
            else
                g_warning("failed to get argument %d: %s", arg, g_strerror(errno));
            return;
        }
        data->resolve = symflags & AT_SYMLINK_NOFOLLOW ? FALSE : TRUE;
    }
    else if (self->flags & IF_AT_REMOVEDIR2) {
        long rmflags;
        if (0 > trace_get_arg(child->pid, 2, &rmflags)) {
            data->result = RS_ERROR;
            data->save_errno = errno;
            if (ESRCH == errno)
                g_debug("failed to get argument 2: %s", g_strerror(errno));
            else
                g_warning("failed to get argument 2: %s", g_strerror(errno));
            return;
        }
        data->resolve = rmflags & AT_REMOVEDIR ? TRUE : FALSE;
    }
    else
        data->resolve = TRUE;
    g_debug("decided %sto resolve symlinks for system call %d, child %i",
            data->resolve ? "" : "not ", self->no, child->pid);
}

static gchar *systemcall_resolvepath(SystemCall *self,
                                 context_t *ctx, struct tchild *child,
                                 int narg, gboolean isat, struct checkdata *data)
{
    gboolean maycreat;
    int mode;
    if (data->open_flags & O_CREAT)
        maycreat = TRUE;
    else if (0 == narg && self->flags & CAN_CREAT)
        maycreat = TRUE;
    else if (1 == narg && (self->flags & CAN_CREAT2 || (isat && self->flags & CAN_CREAT_AT)))
        maycreat = TRUE;
    else if (3 == narg && self->flags & CAN_CREAT_AT2)
        maycreat = TRUE;
    else
        maycreat = FALSE;
    mode = maycreat ? CAN_ALL_BUT_LAST : CAN_EXISTING;

    char *path = data->pathlist[narg];
    char *path_sanitized;
    char *resolved_path;
    char *newcwd = NULL;
    gboolean use_newcwd = FALSE;

    if (isat && NULL != data->dirfdlist[narg - 1]) {
        use_newcwd = TRUE;
        newcwd = data->dirfdlist[narg - 1];
    }
    else if ('/' != path[0] && 0 != strncmp(child->cwd, ctx->cwd, strlen(ctx->cwd) + 1)) {
        if (0 == strncmp(ctx->cwd, child->cwd, strlen(ctx->cwd))) {
            /* child->cwd begins with ctx->cwd, call chdir using relative path
             * instead of absolute path to make sure no errors regarding
             * permissions happen.
             */
            newcwd = child->cwd + strlen(ctx->cwd) + 1;
        }
        else
            newcwd = child->cwd;
    }

    if (NULL != newcwd) {
        if (0 > echdir(newcwd)) {
            g_debug("failed to change current working directory to `%s': %s", newcwd, strerror(errno));
            g_debug("adding current working directory to `%s' instead", path);
            int len = strlen(use_newcwd ? newcwd : child->cwd) + strlen(path) + 2;
            char *pathc = g_malloc(len * sizeof(char));
            snprintf(pathc, len, "%s/%s", use_newcwd ? newcwd : child->cwd, path);
            path_sanitized = remove_slash(pathc);
            g_free(pathc);
        }
        else {
            g_free(ctx->cwd);
            ctx->cwd = isat ? g_strdup(newcwd) : g_strdup(child->cwd);
            path_sanitized = remove_slash(path);
        }
    }
    else
        path_sanitized = remove_slash(path);

    g_debug("mode is %s resolve is %s", maycreat ? "CAN_ALL_BUT_LAST" : "CAN_EXISTING",
                                        data->resolve ? "TRUE" : "FALSE");
    resolved_path = canonicalize_filename_mode(path_sanitized, mode, data->resolve, ctx->cwd);
    if (NULL == resolved_path) {
        data->result = RS_DENY;
        child->retval = -errno;
        g_debug("canonicalize_filename_mode() failed for `%s': %s", path, g_strerror(errno));
    }
    g_free(path_sanitized);
    return resolved_path;
}

static void systemcall_canonicalize(SystemCall *self, gpointer ctx_ptr,
                                    gpointer child_ptr, gpointer data_ptr)
{
    context_t *ctx = (context_t *) ctx_ptr;
    struct tchild *child = (struct tchild *) child_ptr;
    struct checkdata *data = (struct checkdata *) data_ptr;

    if (RS_ALLOW != data->result)
        return;
    else if (!child->sandbox->on)
        return;

    g_debug("canonicalizing paths for system call %d, child %i", self->no, child->pid);

    if (self->flags & CHECK_PATH) {
        g_debug("canonicalizing `%s' for system call %d, child %i", data->pathlist[0], self->no, child->pid);
        data->rpathlist[0] = systemcall_resolvepath(self, ctx, child, 0, FALSE, data);
        if (NULL == data->rpathlist[0])
            return;
        else
            g_debug("canonicalized `%s' to `%s'", data->pathlist[0], data->rpathlist[0]);
    }
    if (self->flags & CHECK_PATH2) {
        g_debug("canonicalizing `%s' for system call %d, child %i", data->pathlist[1], self->no, child->pid);
        data->rpathlist[1] = systemcall_resolvepath(self, ctx, child, 1, FALSE, data);
        if (NULL == data->rpathlist[1])
            return;
        else
            g_debug("canonicalized `%s' to `%s'", data->pathlist[1], data->rpathlist[1]);
    }
    if (self->flags & CHECK_PATH_AT) {
        g_debug("canonicalizing `%s' for system call %d, child %i", data->pathlist[1], self->no, child->pid);
        data->rpathlist[1] = systemcall_resolvepath(self, ctx, child, 1, TRUE, data);
        if (NULL == data->rpathlist[1])
            return;
        else
            g_debug("canonicalized `%s' to `%s'", data->pathlist[1], data->rpathlist[1]);
    }
    if (self->flags & CHECK_PATH_AT2) {
        g_debug("canonicalizing `%s' for system call %d, child %i", data->pathlist[3], self->no, child->pid);
        data->rpathlist[3] = systemcall_resolvepath(self, ctx, child, 3, TRUE, data);
        if (NULL == data->rpathlist[3])
            return;
        else
            g_debug("canonicalized `%s' to `%s'", data->pathlist[3], data->rpathlist[3]);
    }
}

static void systemcall_check_path(SystemCall *self,
                                  context_t *ctx, struct tchild *child,
                                  int narg, struct checkdata *data)
{
    char *path = data->rpathlist[narg];

    g_debug("checking `%s' for write access", path);
    int allow_write = pathlist_check(child->sandbox->write_prefixes, path);
    g_debug("checking `%s' for predict access", path);
    int allow_predict = pathlist_check(child->sandbox->predict_prefixes, path);

    if (!allow_write && !allow_predict) {
        const char *sname;
        char *reason = g_malloc((strlen(path) + 256) * sizeof(char));
        child->retval = -EPERM;
        if (0 == narg)
            strcpy(reason, "%s(\"%s\", ");
        else if (1 == narg)
            strcpy(reason, "%s(?, \"%s\", ");
        else if (3 == narg)
            strcpy(reason, "%s(?, ?, ?, \"%s\", ");

        if (self->flags & ACCESS_MODE)
            strcat(reason, "O_WR)");
        else if (self->flags & OPEN_MODE || self->flags & OPEN_MODE_AT)
            strcat(reason, "O_WRONLY/O_RDWR)");
        else
            strcat(reason, "...)");
        sname = syscall_get_name(self->no);
        access_error(child->pid, reason, sname, path);
        g_free(reason);
        data->result = RS_DENY;
    }
    else if (!allow_write && allow_predict) {
        if (self->flags & RETURNS_FD) {
            g_debug("system call returns fd and its argument is under a predict path");
            g_debug("changing the path argument to /dev/null");
            if (0 > trace_set_string(child->pid, narg, "/dev/null", 10)) {
                data->result = RS_ERROR;
                data->save_errno = errno;
                if (ESRCH == errno)
                    g_debug("failed to set string: %s", g_strerror(errno));
                else
                    g_warning("failed to set string: %s", g_strerror(errno));
            }
        }
        else {
            data->result = RS_DENY;
            child->retval = 0;
        }
    }

    if (sydbox_config_get_paranoid_mode_enabled() && data->resolve) {
        /* Change the path argument with the resolved path to
         * prevent symlink races.
         */
        g_debug ("paranoia! system call resolves symlinks, substituting path with resolved path");
        if (0 > trace_set_string(child->pid, narg, path, strlen(path) + 1)) {
            data->result = RS_ERROR;
            data->save_errno = errno;
            if (ESRCH == errno)
                g_debug("failed to set string: %s", g_strerror(errno));
            else
                g_warning("failed to set string: %s", g_strerror(errno));
        }
    }
}

static void systemcall_check(SystemCall *self, gpointer ctx_ptr,
                             gpointer child_ptr, gpointer data_ptr)
{
    context_t *ctx = (context_t *) ctx_ptr;
    struct tchild *child = (struct tchild *) child_ptr;
    struct checkdata *data = (struct checkdata *) data_ptr;

    if (RS_ALLOW != data->result)
        return;
    else if (!child->sandbox->on)
        return;

    if (self->flags & CHECK_PATH) {
        systemcall_check_path(self, ctx, child, 0, data);
        if (RS_ERROR == data->result || RS_DENY == data->result)
            return;
    }
    if (self->flags & CHECK_PATH2) {
        systemcall_check_path(self, ctx, child, 1, data);
        if (RS_ERROR == data->result || RS_DENY == data->result)
            return;
    }
    if (self->flags & CHECK_PATH_AT) {
        systemcall_check_path(self, ctx, child, 1, data);
        if (RS_ERROR == data->result || RS_DENY == data->result)
            return;
    }
    if (self->flags & CHECK_PATH_AT2) {
        systemcall_check_path(self, ctx, child, 3, data);
        if (RS_ERROR == data->result || RS_DENY == data->result)
            return;
    }
    if (self->flags & NET_CALL && !(child->sandbox->net)) {
#if defined(__NR_socketcall)
        access_error(child->pid, "socketcall()");
#elif defined(__NR_socket)
        access_error(child->pid, "socket()");
#endif
        data->result = RS_DENY;
        child->retval = -EACCES;
    }
}

static void systemcall_end_check(SystemCall *self, gpointer ctx_ptr,
                                 gpointer child_ptr, gpointer data_ptr)
{
    context_t *ctx = (context_t *) ctx_ptr;
    struct tchild *child = (struct tchild *) child_ptr;
    struct checkdata *data = (struct checkdata *) data_ptr;

    g_debug("ending check for system call %d, child %i", self->no, child->pid);
    for (unsigned int i = 0; i < 2; i++)
        g_free(data->dirfdlist[i]);
    for (unsigned int i = 0; i < 4; i++) {
        g_free(data->pathlist[i]);
        g_free(data->rpathlist[i]);
    }
}

static void systemcall_class_init(SystemCallClass *cls) {
    GParamSpec *no, *flags;
    GObjectClass *g_object_cls;

    // Get handle to base object
    g_object_cls = G_OBJECT_CLASS(cls);

    // Set up parameter specs
    no = g_param_spec_uint(
            "no",
            "systemcall-no",
            "system call number",
            0,
            UINT_MAX,
            0,
            G_PARAM_READWRITE);
    flags = g_param_spec_uint(
            "flags",
            "systemcall-flags",
            "system call flags",
            0,
            UINT_MAX,
            0,
            G_PARAM_READWRITE);

    // Override base object methods
    g_object_cls->set_property = systemcall_set_property;
    g_object_cls->get_property = systemcall_get_property;

    // Install properties
    g_object_class_install_property(g_object_cls, PROP_SYSTEMCALL_NO, no);
    g_object_class_install_property(g_object_cls, PROP_SYSTEMCALL_FLAGS, flags);

    // Set signal handlers
    cls->start_check = systemcall_start_check;
    cls->flags = systemcall_flags;
    cls->magic = systemcall_magic;
    cls->resolve = systemcall_resolve;
    cls->canonicalize = systemcall_canonicalize;
    cls->check = systemcall_check;
    cls->end_check = systemcall_end_check;


    // Install signals and default handlers
    systemcall_signal[SIGNAL_CHECK] =
        g_signal_new("check",
                     TYPE_SYSTEMCALL,
                     G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
                     G_STRUCT_OFFSET(SystemCallClass, start_check),
                     NULL,
                     NULL,
                     syscall_marshall_VOID__POINTER_POINTER_POINTER,
                     G_TYPE_NONE,
                     3,
                     G_TYPE_POINTER,
                     G_TYPE_POINTER,
                     G_TYPE_POINTER);
}


GType systemcall_get_type(void) {
    static GType systemcall_type = 0;

    if (0 == systemcall_type) {
        static const GTypeInfo systemcall_info = {
            sizeof(SystemCallClass),
            NULL,
            NULL,
            (GClassInitFunc) systemcall_class_init,
            NULL,
            NULL,
            sizeof(SystemCall),
            16,
            NULL,
            NULL
        };

        systemcall_type = g_type_register_static(
                G_TYPE_OBJECT,
                "SystemCall",
                &systemcall_info,
                0);
    }
    return systemcall_type;
}

void syscall_init(void) {
    static gboolean initialized = FALSE;
    if (initialized)
        return;

    g_type_init();

    int i = 0;
    SystemCall *obj;
#define SYSTEMCALL_HANDLER(_no, _flags)         \
    do {                                        \
        obj = g_object_new(TYPE_SYSTEMCALL,     \
                           "no", (_no),         \
                           "flags", (_flags),   \
                           NULL);               \
        g_signal_connect(obj, "check", (GCallback) systemcall_flags, NULL);         \
        g_signal_connect(obj, "check", (GCallback) systemcall_magic, NULL);         \
        g_signal_connect(obj, "check", (GCallback) systemcall_resolve, NULL);       \
        g_signal_connect(obj, "check", (GCallback) systemcall_canonicalize, NULL);  \
        g_signal_connect(obj, "check", (GCallback) systemcall_check, NULL);         \
        g_signal_connect(obj, "check", (GCallback) systemcall_end_check, NULL);     \
        syscalls[i].no = (_no);     \
        syscalls[i++].obj = obj;    \
    } while (0)

    SYSTEMCALL_HANDLER(__NR_chmod, CHECK_PATH);
    SYSTEMCALL_HANDLER(__NR_chown, CHECK_PATH);
#if defined(__NR_chown32)
    SYSTEMCALL_HANDLER(__NR_chown32, CHECK_PATH);
#endif
    SYSTEMCALL_HANDLER(__NR_open, CHECK_PATH | RETURNS_FD | OPEN_MODE | MAGIC_OPEN);
    SYSTEMCALL_HANDLER(__NR_creat, CHECK_PATH | CAN_CREAT | RETURNS_FD);
    SYSTEMCALL_HANDLER(__NR_stat, MAGIC_STAT);
#if defined(__NR_stat64)
    SYSTEMCALL_HANDLER(__NR_stat64, MAGIC_STAT);
#endif
    SYSTEMCALL_HANDLER(__NR_lchown, CHECK_PATH | DONT_RESOLV);
#if defined(__NR_lchown32)
    SYSTEMCALL_HANDLER(__NR_lchown32, CHECK_PATH | DONT_RESOLV);
#endif
    SYSTEMCALL_HANDLER(__NR_link, CHECK_PATH | CHECK_PATH2 | CAN_CREAT2 | DONT_RESOLV);
    SYSTEMCALL_HANDLER(__NR_mkdir, CHECK_PATH | CAN_CREAT);
    SYSTEMCALL_HANDLER(__NR_mknod, CHECK_PATH | CAN_CREAT);
    SYSTEMCALL_HANDLER(__NR_access, CHECK_PATH | ACCESS_MODE);
    SYSTEMCALL_HANDLER(__NR_rename, CHECK_PATH | CHECK_PATH2 | CAN_CREAT2);
    SYSTEMCALL_HANDLER(__NR_rmdir, CHECK_PATH);
    SYSTEMCALL_HANDLER(__NR_symlink, CHECK_PATH2 | CAN_CREAT2 | DONT_RESOLV);
    SYSTEMCALL_HANDLER(__NR_truncate, CHECK_PATH);
#if defined(__NR_truncate64)
    SYSTEMCALL_HANDLER(__NR_truncate64, CHECK_PATH);
#endif
    SYSTEMCALL_HANDLER(__NR_mount, CHECK_PATH2);
#if defined(__NR_umount)
    SYSTEMCALL_HANDLER(__NR_umount, CHECK_PATH);
#endif
    SYSTEMCALL_HANDLER(__NR_umount2, CHECK_PATH);
    SYSTEMCALL_HANDLER(__NR_utime, CHECK_PATH);
    SYSTEMCALL_HANDLER(__NR_unlink, CHECK_PATH | DONT_RESOLV);
    SYSTEMCALL_HANDLER(__NR_openat, CHECK_PATH_AT | OPEN_MODE_AT | RETURNS_FD);
    SYSTEMCALL_HANDLER(__NR_mkdirat, CHECK_PATH_AT | CAN_CREAT_AT);
    SYSTEMCALL_HANDLER(__NR_mknodat, CHECK_PATH_AT | CAN_CREAT_AT);
    SYSTEMCALL_HANDLER(__NR_fchownat, CHECK_PATH_AT | IF_AT_SYMLINK_NOFOLLOW4);
    SYSTEMCALL_HANDLER(__NR_unlinkat, CHECK_PATH_AT | IF_AT_REMOVEDIR2);
    SYSTEMCALL_HANDLER(__NR_renameat, CHECK_PATH_AT | CHECK_PATH_AT2 | CAN_CREAT_AT2);
    SYSTEMCALL_HANDLER(__NR_linkat, CHECK_PATH_AT | CHECK_PATH_AT2 | CAN_CREAT_AT2 | IF_AT_SYMLINK_FOLLOW4);
    SYSTEMCALL_HANDLER(__NR_symlinkat, CHECK_PATH_AT | CHECK_PATH_AT2 | CAN_CREAT_AT2 | DONT_RESOLV);
    SYSTEMCALL_HANDLER(__NR_fchmodat, CHECK_PATH_AT | IF_AT_SYMLINK_NOFOLLOW3);
    SYSTEMCALL_HANDLER(__NR_faccessat, CHECK_PATH_AT | ACCESS_MODE_AT);
#if defined(__NR_socketcall)
    SYSTEMCALL_HANDLER(__NR_socketcall, NET_CALL);
#elif defined(__NR_socket)
    SYSTEMCALL_HANDLER(__NR_socket, NET_CALL);
#endif

#undef SYSTEMCALL_HANDLER

    syscalls[i].no = -1;
    syscalls[i].obj = NULL;

    initialized = TRUE;
}

void syscall_free(void) {
    for (unsigned int i = 0; NULL != syscalls[i].obj; i++) {
        g_object_unref(syscalls[i].obj);
        syscalls[i].obj = NULL;
    }
}

SystemCall *syscall_get_handler(int no) {
    for (unsigned int i = 0; NULL != syscalls[i].obj; i++) {
        if (syscalls[i].no == no)
            return syscalls[i].obj;
    }
    return NULL;
}

int syscall_handle(context_t *ctx, struct tchild *child) {
    long sno;
    const char *sname;
    struct checkdata data;
    SystemCall *handler;

    if (0 > trace_get_syscall(child->pid, &sno)) {
        if (errno != ESRCH) {
            g_printerr ("failed to get syscall: %s", g_strerror (errno));
            exit (-1);
        }
        return handle_esrch(ctx, child);
    }

    sname = (0xbadca11 == sno) ? syscall_get_name(child->sno) : syscall_get_name(sno);
    if (NULL == sname)
        sname = DEFAULT_NAME;

    if (!(child->flags & TCHILD_INSYSCALL)) { // Entering syscall
        g_log(G_LOG_DOMAIN, LOG_LEVEL_DEBUG_TRACE, "child %i is entering system call %s()", child->pid, sname);
        handler = syscall_get_handler(sno);
        if (NULL == handler) // Safe system call
            g_log(G_LOG_DOMAIN, LOG_LEVEL_DEBUG_TRACE, "allowing access to system call %s()", sname);
        else {
            memset(&data, 0, sizeof(struct checkdata));
            g_signal_emit_by_name(handler, "check", ctx, child, &data);

            switch(data.result) {
                case RS_DENY:
                    g_debug("denying access to system call %s()", sname);
                    child->sno = sno;
                    if (0 > trace_set_syscall(child->pid, 0xbadca11)) {
                        if (ESRCH != errno) {
                            g_printerr("failed to set syscall: %s", g_strerror(errno));
                            exit(-1);
                        }
                        return handle_esrch(ctx, child);
                    }
                    break;
                case RS_ALLOW:
                case RS_NOWRITE:
                case RS_MAGIC:
                    g_log(G_LOG_DOMAIN, LOG_LEVEL_DEBUG_TRACE, "allowing access to system call %s()", sname);
                    break;
                case RS_ERROR:
                    if (ESRCH != errno) {
                        g_printerr("error while checking system call %s() for access: %s", sname, g_strerror(errno));
                        exit(-1);
                    }
                    return handle_esrch(ctx, child);
                    break;
                default:
                    g_assert_not_reached();
                    break;
            }
        }
    }
    else { // Exiting sytem call
        g_log(G_LOG_DOMAIN, LOG_LEVEL_DEBUG_TRACE, "child %i is exiting system call %s()", child->pid, sname);
        if (0xbadca11 == sno) {
            g_debug("restoring real call number for denied system call %s()", sname);
            // Restore real call number and return our error code
            if (0 > trace_set_syscall(child->pid, child->sno)) {
                if (ESRCH != errno) {
                    g_printerr("failed to restore syscall: %s", g_strerror (errno));
                    exit(-1);
                }
                return handle_esrch(ctx, child);
            }
            if (0 > trace_set_return(child->pid, child->retval)) {
                if (ESRCH != errno) {
                    g_printerr("failed to set return code: %s", g_strerror(errno));
                    exit(-1);
                }
                return handle_esrch(ctx, child);
            }
        }
        else if (__NR_chdir == sno || __NR_fchdir == sno) {
            long retval;
            if (0 > trace_get_return(child->pid, &retval)) {
                if (ESRCH != errno) {
                    g_printerr("failed to get return code: %s", g_strerror (errno));
                    exit(-1);
                }
                return handle_esrch(ctx, child);
            }
            if (0 == retval) {
                // Child has successfully changed directory
                char *newcwd = pgetcwd(ctx, child->pid);
                if (NULL == newcwd) {
                    retval = -errno;
                    g_debug("pgetcwd() failed: %s", g_strerror(errno));
                    if (0 > trace_set_return(child->pid, retval)) {
                        if (ESRCH != errno) {
                            g_printerr("failed to set return code: %s", g_strerror (errno));
                            exit(-1);
                        }
                        return handle_esrch(ctx, child);
                    }
                }
                else {
                    if (NULL != child->cwd)
                        g_free (child->cwd);
                    child->cwd = newcwd;
                    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO,
                            "child %i has changed directory to '%s'", child->pid, child->cwd);
                }
            }
            else
                g_debug("child %i failed to change directory: %s", child->pid, g_strerror(-retval));
        }

    }
    child->flags ^= TCHILD_INSYSCALL;
    return 0;
}
