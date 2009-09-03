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
#endif // !_ATFILE_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <sys/socket.h>

#include <glib.h>
#include <glib-object.h>

#include "net.h"
#include "path.h"
#include "proc.h"
#include "trace.h"
#include "wrappers.h"
#include "syscall_marshaller.h"
#include "syscall.h"

#include "sydbox-log.h"
#include "sydbox-utils.h"
#include "sydbox-config.h"

#include "flags.h"
#include "dispatch.h"

#define BAD_SYSCALL                 0xbadca11
#define IS_SUPPORTED_FAMILY(f)      ((f) == AF_UNIX || (f) == AF_INET || (f) == AF_INET6)
#define IS_NET_CALL(fl)             ((fl) & (BIND_CALL | CONNECT_CALL | SENDTO_CALL | DECODE_SOCKETCALL))
#define NET_RESTRICTED_CALL(fl)     ((fl) & (CONNECT_CALL | SENDTO_CALL))
#define NET_RESTRICTED_SUBCALL(sub) ((sub) == SOCKET_SUBCALL_CONNECT || (sub) == SOCKET_SUBCALL_SENDTO)

#define MODE_STRING(flags)                                                      \
    ((flags) & OPEN_MODE || (flags) & OPEN_MODE_AT) ? "O_WRONLY/O_RDWR" : "..."

enum {
    PROP_SYSTEMCALL_0,
    PROP_SYSTEMCALL_NO,
    PROP_SYSTEMCALL_FLAGS,
};

static SystemCall *SystemCallHandler;
static const char *sname;

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

/* Receive the path argument at position narg of child with given pid and
 * update data.
 * Returns FALSE and sets data->result to RS_ERROR and data->save_errno to
 * errno on failure.
 * Returns TRUE and updates data->pathlist[narg] on success.
 */
static bool systemcall_get_path(pid_t pid, int personality, int narg, struct checkdata *data)
{
    data->pathlist[narg] = trace_get_path(pid, personality, narg);
    if (G_UNLIKELY(NULL == data->pathlist[narg])) {
        data->result = RS_ERROR;
        data->save_errno = errno;
        if (ESRCH == errno || EIO == errno || EFAULT == errno)
            g_debug("failed to grab string from argument %d: %s", narg, g_strerror(errno));
        else
            g_warning("failed to grab string from argument %d: %s", narg, g_strerror(errno));
        return false;
    }
    g_debug("path argument %d is `%s'", narg, data->pathlist[narg]);
    return true;
}

/* Receive dirfd argument at position narg of the given child and update data.
 * Returns FALSE and sets data->result to RS_ERROR and data->save_errno to
 * errno on failure.
 * If dirfd is AT_FDCWD it copies child->cwd to data->dirfdlist[narg].
 * Otherwise tries to determine the directory using pgetdir().
 * If pgetdir() fails it sets data->result to RS_DENY and child->retval to
 * -errno and returns FALSE.
 * On success TRUE is returned and data->dirfdlist[narg] contains the directory
 * information about dirfd. This string should be freed after use.
 */
static bool systemcall_get_dirfd(SystemCall *self,
                                 struct tchild *child,
                                 int narg, struct checkdata *data)
{
    long dfd;
    if (G_UNLIKELY(0 > trace_get_arg(child->pid, child->personality, narg, &dfd))) {
        data->result = RS_ERROR;
        data->save_errno = errno;
        if (ESRCH == errno)
            g_debug("failed to get dirfd from argument %d: %s", narg, g_strerror(errno));
        else
            g_warning("failed to get dirfd from argument %d: %s", narg, g_strerror(errno));
        return false;
    }

    if (AT_FDCWD != dfd) {
        data->dirfdlist[narg] = pgetdir(child->pid, dfd);
        if (NULL == data->dirfdlist[narg]) {
            data->result = RS_DENY;
            child->retval = -errno;
            g_debug("pgetdir() failed: %s", g_strerror(errno));
            g_debug("denying access to system call %d(%s)", self->no, sname);
            return false;
        }
    }
    else
        data->dirfdlist[narg] = g_strdup(child->cwd);
    return true;
}

/* Initial callback for system call handler.
 * Updates struct checkdata with path and dirfd information.
 */
static void systemcall_start_check(SystemCall *self, gpointer ctx_ptr,
                                   gpointer child_ptr, gpointer data_ptr)
{
    context_t *ctx = (context_t *) ctx_ptr;
    struct tchild *child = (struct tchild *) child_ptr;
    struct checkdata *data = (struct checkdata *) data_ptr;

    g_debug("starting check for system call %d(%s), child %i", self->no, sname, child->pid);
    if (self->flags & CHECK_PATH || self->flags & MAGIC_STAT) {
        if (!systemcall_get_path(child->pid, child->personality, 0, data))
            return;
    }
    if (self->flags & CHECK_PATH2) {
        if (!systemcall_get_path(child->pid, child->personality, 1, data))
            return;
    }
    if (self->flags & CHECK_PATH_AT) {
        if (!systemcall_get_dirfd(self, child, 0, data))
            return;
        if (!systemcall_get_path(child->pid, child->personality, 1, data))
            return;
    }
    if (self->flags & CHECK_PATH_AT1) {
        if (!systemcall_get_dirfd(self, child, 1, data))
            return;
        if (!systemcall_get_path(child->pid, child->personality, 2, data))
            return;
    }
    if (self->flags & CHECK_PATH_AT2) {
        if (!systemcall_get_dirfd(self, child, 2, data))
            return;
        if (!systemcall_get_path(child->pid, child->personality, 3, data))
            return;
    }
    if (!ctx->before_initial_execve && child->sandbox->exec && self->flags & EXEC_CALL) {
        if (!systemcall_get_path(child->pid, child->personality, 0, data))
            return;
    }
    if (child->sandbox->network && child->sandbox->network_mode != SYDBOX_NETWORK_ALLOW) {
        if (self->flags & DECODE_SOCKETCALL) {
            data->socket_subcall = trace_decode_socketcall(child->pid, child->personality);
            if (0 > data->socket_subcall) {
                data->result = RS_ERROR;
                data->save_errno = errno;
                return;
            }
            g_debug("Decoded socket subcall is %d", data->socket_subcall);
            if (data->socket_subcall == SOCKET_SUBCALL_SOCKET)
                sname = "socket";
            else if (data->socket_subcall == SOCKET_SUBCALL_BIND || data->socket_subcall == SOCKET_SUBCALL_CONNECT) {
                sname = (data->socket_subcall == SOCKET_SUBCALL_BIND) ? "bind" : "connect";
                data->addr = trace_get_addr(child->pid, child->personality, 1, true, &(data->family), &(data->port));
                if (data->addr == NULL) {
                    data->result = RS_ERROR;
                    data->save_errno = errno;
                    return;
                }
                g_debug("Destination of %s subcall family:%d addr:%s port:%d",
                            sname, data->family, data->addr, data->port);
            }
            else if (data->socket_subcall == SOCKET_SUBCALL_SENDTO) {
                sname = "sendto";
                data->addr = trace_get_addr(child->pid, child->personality, 4, true, &(data->family), &(data->port));
                if (data->addr == NULL) {
                    data->result = RS_ERROR;
                    data->save_errno = errno;
                    return;
                }
                g_debug("Destination of %s subcall family:%d addr:%s port:%d",
                            sname, data->family, data->addr, data->port);
            }
        }
        else if (self->flags & (BIND_CALL | CONNECT_CALL)) {
            data->addr = trace_get_addr(child->pid, child->personality, 1, false, &(data->family), &(data->port));
            if (data->addr == NULL) {
                data->result = RS_ERROR;
                data->save_errno = errno;
                return;
            }
            g_debug("Destination of %s call family:%d addr:%s port:%d", sname, data->family, data->addr, data->port);
        }
        else if (self->flags & SENDTO_CALL) {
            data->addr = trace_get_addr(child->pid, child->personality, 4, false, &(data->family), &(data->port));
            if (data->addr == NULL) {
                data->result = RS_ERROR;
                data->save_errno = errno;
                return;
            }
            g_debug("Destination of %s call family:%d addr:%s port:%d", sname, data->family, data->addr, data->port);
        }
    }
}

/* Second callback for system call handler
 * Checks the flag arguments of system calls.
 * If data->result isn't RS_ALLOW, which means an error has occured in a
 * previous callback or a decision has been made, it does nothing and simply
 * returns.
 * If the system call isn't one of open, openat, access or accessat, it does
 * nothing and simply returns.
 * If an error occurs during flag checking it sets data->result to RS_ERROR,
 * data->save_errno to errno and returns.
 * If the flag doesn't have O_CREAT, O_WRONLY or O_RDWR set for system call
 * open or openat it sets data->result to RS_NOWRITE and returns.
 * If the flag doesn't have W_OK set for system call access or accessat it
 * sets data->result to RS_NOWRITE and returns.
 */
static void systemcall_flags(SystemCall *self, gpointer ctx_ptr G_GNUC_UNUSED,
                             gpointer child_ptr, gpointer data_ptr)
{
    struct tchild *child = (struct tchild *) child_ptr;
    struct checkdata *data = (struct checkdata *) data_ptr;

    if (G_UNLIKELY(RS_ALLOW != data->result))
        return;
    else if (!(self->flags & OPEN_MODE || self->flags & OPEN_MODE_AT
                || self->flags & ACCESS_MODE || self->flags & ACCESS_MODE_AT))
        return;

    if (self->flags & OPEN_MODE || self->flags & OPEN_MODE_AT) {
        int arg = self->flags & OPEN_MODE ? 1 : 2;
        if (G_UNLIKELY(0 > trace_get_arg(child->pid, child->personality, arg, &(data->open_flags)))) {
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
        if (G_UNLIKELY(0 > trace_get_arg(child->pid, child->personality, arg, &(data->access_flags)))) {
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

/* Checks for magic stat() calls.
 * If the stat() call is magic, this function calls trace_fake_stat() to fake
 * the stat buffer and sets data->result to RS_DENY and child->retval to 0.
 * If trace_fake_stat() fails it sets data->result to RS_ERROR and
 * data->save_errno to errno.
 * If the stat() call isn't magic, this function does nothing.
 */
static void systemcall_magic_stat(struct tchild *child, struct checkdata *data)
{
    char *path = data->pathlist[0];
    const char *rpath;
    char *rpath_sanitized;
    GSList *whitelist;

    g_debug("checking if stat(\"%s\") is magic", path);
    if (G_LIKELY(!path_magic_dir(path))) {
        g_debug("stat(\"%s\") not magic", path);
        return;
    }

    if (path_magic_on(path)) {
        data->result = RS_MAGIC;
        child->sandbox->path = true;
        g_info("path sandboxing is now enabled for child %i", child->pid);
    }
    else if (path_magic_off(path)) {
        data->result = RS_MAGIC;
        child->sandbox->path = false;
        g_info("path sandboxing is now disabled for child %i", child->pid);
    }
    else if (path_magic_toggle(path)) {
        data->result = RS_MAGIC;
        child->sandbox->path = !(child->sandbox->path);
        g_info("path sandboxing is now %sabled for child %i", child->sandbox->path ? "en" : "dis", child->pid);
    }
    else if (path_magic_lock(path)) {
        data->result = RS_MAGIC;
        child->sandbox->lock = LOCK_SET;
        g_info("access to magic commands is now denied for child %i", child->pid);
    }
    else if (path_magic_exec_lock(path)) {
        data->result = RS_MAGIC;
        child->sandbox->lock = LOCK_PENDING;
        g_info("access to magic commands will be denied on execve() for child %i", child->pid);
    }
    else if (path_magic_wait_all(path)) {
        data->result = RS_MAGIC;
        sydbox_config_set_wait_all(true);
        g_info("tracing will be finished when all children exit");
    }
    else if (path_magic_wait_eldest(path)) {
        data->result = RS_MAGIC;
        sydbox_config_set_wait_all(false);
        g_info("tracing will be finished when the eldest child exits");
    }
    else if (path_magic_write(path)) {
        data->result = RS_MAGIC;
        rpath = path + CMD_WRITE_LEN;
        pathnode_new(&(child->sandbox->write_prefixes), rpath, 1);
        g_info("approved addwrite(\"%s\") for child %i", rpath, child->pid);
    }
    else if (path_magic_rmwrite(path)) {
        data->result = RS_MAGIC;
        rpath = path + CMD_RMWRITE_LEN;
        rpath_sanitized = sydbox_compress_path(rpath);
        if (NULL != child->sandbox->write_prefixes)
            pathnode_delete(&(child->sandbox->write_prefixes), rpath_sanitized);
        g_info("approved rmwrite(\"%s\") for child %i", rpath_sanitized, child->pid);
        g_free(rpath_sanitized);
    }
    else if (path_magic_sandbox_exec(path)) {
        data->result = RS_MAGIC;
        child->sandbox->exec = true;
        g_info("execve(2) sandboxing is now enabled for child %i", child->pid);
    }
    else if (path_magic_sandunbox_exec(path)) {
        data->result = RS_MAGIC;
        child->sandbox->exec = false;
        g_info("execve(2) sandboxing is now disabled for child %i", child->pid);
    }
    else if (path_magic_addexec(path)) {
        data->result = RS_MAGIC;
        rpath = path + CMD_ADDEXEC_LEN;
        pathnode_new(&(child->sandbox->exec_prefixes), rpath, 1);
        g_info("approved addexec(\"%s\") for child %i", rpath, child->pid);
    }
    else if (path_magic_rmexec(path)) {
        data->result = RS_MAGIC;
        rpath = path + CMD_RMEXEC_LEN;
        rpath_sanitized = sydbox_compress_path(rpath);
        if (NULL != child->sandbox->exec_prefixes)
            pathnode_delete(&(child->sandbox->exec_prefixes), rpath_sanitized);
        g_info("approved rmexec(\"%s\") for child %i", rpath_sanitized, child->pid);
        g_free(rpath_sanitized);
    }
    else if (path_magic_sandbox_net(path)) {
        data->result = RS_MAGIC;
        child->sandbox->network = true;
        g_info("network sandboxing is now enabled for child %i", child->pid);
    }
    else if (path_magic_sandunbox_net(path)) {
        data->result = RS_MAGIC;
        child->sandbox->network = false;
        g_info("network sandboxing is now disabled for child %i", child->pid);
    }
    else if (path_magic_addfilter(path)) {
        data->result = RS_MAGIC;
        rpath = path + CMD_ADDFILTER_LEN;
        sydbox_config_addfilter(rpath);
        g_info("approved addfilter(\"%s\") for child %i", rpath, child->pid);
    }
    else if (path_magic_rmfilter(path)) {
        data->result = RS_MAGIC;
        rpath = path + CMD_RMFILTER_LEN;
        sydbox_config_rmfilter(rpath);
        g_info("approved rmfilter(\"%s\") for child %i", rpath, child->pid);
    }
    else if (path_magic_net_allow(path)) {
        data->result = RS_MAGIC;
        child->sandbox->network_mode = SYDBOX_NETWORK_ALLOW;
        g_info("approved net.allow() for child %i", child->pid);
    }
    else if (path_magic_net_deny(path)) {
        data->result = RS_MAGIC;
        child->sandbox->network_mode = SYDBOX_NETWORK_DENY;
        g_info("approved net.deny() for child %i", child->pid);
    }
    else if (path_magic_net_local(path)) {
        data->result = RS_MAGIC;
        child->sandbox->network_mode = SYDBOX_NETWORK_LOCAL;
        g_info("approved net.local() for child %i", child->pid);
    }
    else if (path_magic_net_restrict_connect(path)) {
        data->result = RS_MAGIC;
        child->sandbox->network_restrict_connect = true;
        g_info("approved net.restrict.connect() for child %i", child->pid);
    }
    else if (path_magic_net_unrestrict_connect(path)) {
        data->result = RS_MAGIC;
        child->sandbox->network_restrict_connect = false;
        g_info("approved net.unrestrict.connect() for child %i", child->pid);
    }
    else if (path_magic_net_whitelist(path)) {
        data->result = RS_MAGIC;
        whitelist = sydbox_config_get_network_whitelist();
        rpath = path + CMD_NET_WHITELIST_LEN;
        if (0 > netlist_new_from_string(&whitelist, rpath, true))
            g_warning("malformed whitelist address `%s'", rpath);
        else
            sydbox_config_set_network_whitelist(whitelist);
    }
    else if (child->sandbox->path || !path_magic_enabled(path))
        data->result = RS_MAGIC;

    if (data->result == RS_MAGIC) {
        g_debug("stat(\"%s\") is magic, faking stat buffer", path);
        if (G_UNLIKELY(0 > trace_fake_stat(child->pid, child->personality))) {
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

/* Third callback for system call handler.
 * Checks for magic calls.
 * If data->result isn't RS_ALLOW, which means an error has occured in a
 * previous callback or a decision has been made, it does nothing and simply
 * returns.
 * If child->sandbox->lock is set to LOCK_SET which means magic calls are
 * locked, it does nothing and simply returns.
 * If the system call isn't stat(), it does nothing and simply returns.
 * Otherwise it calls systemcall_magic_stat()
 */
static void systemcall_magic(SystemCall *self, gpointer ctx_ptr G_GNUC_UNUSED,
                             gpointer child_ptr, gpointer data_ptr)
{
    struct tchild *child = (struct tchild *) child_ptr;
    struct checkdata *data = (struct checkdata *) data_ptr;

    if (G_UNLIKELY(RS_ALLOW != data->result))
        return;
    else if (LOCK_SET == child->sandbox->lock) {
        g_debug("Lock is set for child %i, skipping magic checks", child->pid);
        return;
    }
    else if (!(self->flags & MAGIC_STAT))
        return;

    systemcall_magic_stat(child, data);
}

/* Fourth callback for systemcall handler.
 * Checks whether symlinks should be resolved for the given system call
 * If data->result isn't RS_ALLOW, which means an error has occured in a
 * previous callback or a decision has been made, it does nothing and simply
 * returns.
 * If child->sandbox->path is false it does nothing and simply returns.
 * If everything was successful this function sets data->resolve to a boolean
 * which gives information about whether the symlinks should be resolved.
 * On failure this function sets data->result to RS_ERROR and data->save_errno
 * to errno.
 */
static void systemcall_resolve(SystemCall *self, gpointer ctx_ptr G_GNUC_UNUSED,
                               gpointer child_ptr, gpointer data_ptr)
{
    struct tchild *child = (struct tchild *) child_ptr;
    struct checkdata *data = (struct checkdata *) data_ptr;

    if (G_UNLIKELY(RS_ALLOW != data->result))
        return;
    else if (child->sandbox->exec && self->flags & EXEC_CALL)
        data->resolve = true;
    else if (!child->sandbox->path)
        return;

    g_debug("deciding whether we should resolve symlinks for system call %d(%s), child %i",
            self->no, sname, child->pid);
    if (self->flags & DONT_RESOLV)
        data->resolve = false;
    else if (self->flags & IF_AT_SYMLINK_FOLLOW4) {
        long symflags;
        if (G_UNLIKELY(0 > trace_get_arg(child->pid, child->personality, 4, &symflags))) {
            data->result = RS_ERROR;
            data->save_errno = errno;
            if (ESRCH == errno)
                g_debug("failed to get argument 4: %s", g_strerror(errno));
            else
                g_warning("failed to get argument 4: %s", g_strerror(errno));
            return;
        }
        data->resolve = symflags & AT_SYMLINK_FOLLOW ? true : false;
    }
    else if (self->flags & IF_AT_SYMLINK_NOFOLLOW3 || self->flags & IF_AT_SYMLINK_NOFOLLOW4) {
        long symflags;
        int arg = self->flags & IF_AT_SYMLINK_NOFOLLOW3 ? 3 : 4;
        if (G_UNLIKELY(0 > trace_get_arg(child->pid, child->personality, arg, &symflags))) {
            data->result = RS_ERROR;
            data->save_errno = errno;
            if (ESRCH == errno)
                g_debug("failed to get argument %d: %s", arg, g_strerror(errno));
            else
                g_warning("failed to get argument %d: %s", arg, g_strerror(errno));
            return;
        }
        data->resolve = symflags & AT_SYMLINK_NOFOLLOW ? false : true;
    }
    else if (self->flags & IF_AT_REMOVEDIR2) {
        long rmflags;
        if (G_UNLIKELY(0 > trace_get_arg(child->pid, child->personality, 2, &rmflags))) {
            data->result = RS_ERROR;
            data->save_errno = errno;
            if (ESRCH == errno)
                g_debug("failed to get argument 2: %s", g_strerror(errno));
            else
                g_warning("failed to get argument 2: %s", g_strerror(errno));
            return;
        }
        data->resolve = rmflags & AT_REMOVEDIR ? true : false;
    }
    else
        data->resolve = true;
    g_debug("decided %sto resolve symlinks for system call %d(%s), child %i",
            data->resolve ? "" : "not ", self->no, sname, child->pid);
}

/* Resolves path for system calls
 * This function calls canonicalize_filename_mode() after sanitizing path
 * On success it returns resolved path.
 * On failure it sets data->result to RS_DENY and child->retval to -errno.
 */
static gchar *systemcall_resolvepath(SystemCall *self,
                                 struct tchild *child,
                                 int narg, bool isat, struct checkdata *data)
{
    bool maycreat;
    int mode;
    if (data->open_flags & O_CREAT)
        maycreat = true;
    else if (0 == narg && self->flags & (CAN_CREAT | MUST_CREAT))
        maycreat = true;
    else if (1 == narg && self->flags & (CAN_CREAT2 | MUST_CREAT2))
        maycreat = true;
    else if (1 == narg && isat && self->flags & (CAN_CREAT_AT | MUST_CREAT_AT))
        maycreat = true;
    else if (2 == narg && isat && self->flags & MUST_CREAT_AT1)
        maycreat = true;
    else if (3 == narg && self->flags & (CAN_CREAT_AT2 | MUST_CREAT_AT2))
        maycreat = true;
    else
        maycreat = false;
    mode = maycreat ? CAN_ALL_BUT_LAST : CAN_EXISTING;

    char *path = data->pathlist[narg];
    char *path_sanitized;
    char *resolved_path;

    if (!g_path_is_absolute(path)) {
        char *absdir, *abspath;
        if (isat && NULL != data->dirfdlist[narg - 1]) {
            absdir = data->dirfdlist[narg - 1];
            g_debug("adding dirfd `%s' to `%s' to make it an absolute path", absdir, path);
        }
        else {
            absdir = child->cwd;
            g_debug("adding current working directory `%s' to `%s' to make it an absolute path", absdir, path);
        }

        abspath = g_build_path(G_DIR_SEPARATOR_S, absdir, path, NULL);
        path_sanitized = sydbox_compress_path(abspath);
        g_free(abspath);
    }
    else
        path_sanitized = sydbox_compress_path (path);

#ifdef HAVE_PROC_SELF
    /* Special case for /proc/self.
     * This symbolic link resolves to /proc/PID, if we let
     * canonicalize_filename_mode() resolve this, we'll get a different result.
     */
    if (0 == strncmp(path_sanitized, "/proc/self", 10)) {
        g_debug("substituting /proc/self with /proc/%i", child->pid);
        char *tmp = g_malloc(strlen(path_sanitized) + 32);
        snprintf(tmp, strlen(path_sanitized) + 32, "/proc/%i/%s", child->pid, path_sanitized + 10);
        g_free(path_sanitized);
        path_sanitized = sydbox_compress_path(tmp);
        g_free(tmp);
    }
#endif

    g_debug("mode is %s resolve is %s", maycreat ? "CAN_ALL_BUT_LAST" : "CAN_EXISTING",
                                        data->resolve ? "TRUE" : "FALSE");
    resolved_path = canonicalize_filename_mode(path_sanitized, mode, data->resolve);
    if (NULL == resolved_path) {
        data->result = RS_DENY;
        child->retval = -errno;
        g_debug("canonicalize_filename_mode() failed for `%s': %s", path, g_strerror(errno));
    }
    g_free(path_sanitized);
    return resolved_path;
}

/* Fifth callback for system call handler.
 * Canonicalizes path arguments.
 * If data->result isn't RS_ALLOW, which means an error has occured in a
 * previous callback or a decision has been made, it does nothing and simply
 * returns.
 * If child->sandbox->path is false it does nothing and simply returns.
 */
static void systemcall_canonicalize(SystemCall *self, gpointer ctx_ptr,
                                    gpointer child_ptr, gpointer data_ptr)
{
    context_t *ctx = (context_t *) ctx_ptr;
    struct tchild *child = (struct tchild *) child_ptr;
    struct checkdata *data = (struct checkdata *) data_ptr;

    if (G_UNLIKELY(RS_ALLOW != data->result))
        return;

    if (!ctx->before_initial_execve && child->sandbox->exec && self->flags & EXEC_CALL) {
        g_debug("canonicalizing `%s' for system call %d(%s), child %i", data->pathlist[0],
                self->no, sname, child->pid);
        data->rpathlist[0] = systemcall_resolvepath(self, child, 0, TRUE, data);
        if (NULL == data->rpathlist[0])
            return;
        else
            g_debug("canonicalized `%s' to `%s'", data->pathlist[0], data->rpathlist[0]);
    }

    if (!child->sandbox->path)
        return;
    if (self->flags & CHECK_PATH) {
        g_debug("canonicalizing `%s' for system call %d(%s), child %i", data->pathlist[0],
                self->no, sname, child->pid);
        data->rpathlist[0] = systemcall_resolvepath(self, child, 0, FALSE, data);
        if (NULL == data->rpathlist[0])
            return;
        else
            g_debug("canonicalized `%s' to `%s'", data->pathlist[0], data->rpathlist[0]);
    }
    if (self->flags & CHECK_PATH2) {
        g_debug("canonicalizing `%s' for system call %d(%s), child %i", data->pathlist[1],
                self->no, sname, child->pid);
        data->rpathlist[1] = systemcall_resolvepath(self, child, 1, FALSE, data);
        if (NULL == data->rpathlist[1])
            return;
        else
            g_debug("canonicalized `%s' to `%s'", data->pathlist[1], data->rpathlist[1]);
    }
    if (self->flags & CHECK_PATH_AT) {
        g_debug("canonicalizing `%s' for system call %d(%s), child %i", data->pathlist[1],
                self->no, sname, child->pid);
        data->rpathlist[1] = systemcall_resolvepath(self, child, 1, TRUE, data);
        if (NULL == data->rpathlist[1])
            return;
        else
            g_debug("canonicalized `%s' to `%s'", data->pathlist[1], data->rpathlist[1]);
    }
    if (self->flags & CHECK_PATH_AT1) {
        g_debug("canonicalizing `%s' for system call %d(%s), child %i", data->pathlist[2],
                self->no, sname, child->pid);
        data->rpathlist[2] = systemcall_resolvepath(self, child, 2, TRUE, data);
        if (NULL == data->rpathlist[2])
            return;
        else
            g_debug("canonicalized `%s' to `%s'", data->pathlist[2], data->rpathlist[2]);
    }
    if (self->flags & CHECK_PATH_AT2) {
        g_debug("canonicalizing `%s' for system call %d(%s), child %i", data->pathlist[3],
                self->no, sname, child->pid);
        data->rpathlist[3] = systemcall_resolvepath(self, child, 3, TRUE, data);
        if (NULL == data->rpathlist[3])
            return;
        else
            g_debug("canonicalized `%s' to `%s'", data->pathlist[3], data->rpathlist[3]);
    }
}

static int systemcall_check_create(SystemCall *self,
                                   struct tchild *child,
                                   int narg, struct checkdata *data)
{
    char *path;
    struct stat buf;

    path = data->rpathlist[narg];
    if ((narg == 0 && self->flags & MUST_CREAT) ||
            (narg == 1 && self->flags & (MUST_CREAT2 | MUST_CREAT_AT)) ||
            (narg == 3 && self->flags & MUST_CREAT_AT2)) {
        g_debug("system call %d(%s) has one of MUST_CREAT* flags set, checking if `%s' exists",
                self->no, sname, path);
        if (0 == stat(path, &buf)) {
            /* The system call _has_ to create the path but it exists.
             * Deny the system call and set errno to EEXIST but don't throw
             * an access violation.
             * Useful for cases like mkdir -p a/b/c.
             */
            g_debug("`%s' exists, system call %d(%s) will fail with EEXIST", path, self->no, sname);
            g_debug("denying system call %d(%s) and failing with EEXIST without violation", self->no, sname);
            data->result = RS_DENY;
            child->retval = -EEXIST;
            return 1;
        }
    }
    return 0;
}

static void systemcall_check_path(SystemCall *self,
                                  struct tchild *child,
                                  int narg, struct checkdata *data)
{
    char *path = data->rpathlist[narg];

    g_debug("checking `%s' for write access", path);
    int allow_write = pathlist_check(child->sandbox->write_prefixes, path);

    if (G_UNLIKELY(!allow_write)) {
        if (systemcall_check_create(self, child, narg, data))
            return;

        child->retval = -EPERM;

        /* Don't raise access violations for access(2) system call.
         * Silently deny it instead.
         */
        if (self->flags & ACCESS_MODE) {
            data->result = RS_DENY;
            return;
        }

        switch (narg) {
            case 0:
                sydbox_access_violation(child->pid, path, "%s(\"%s\", %s)",
                                        sname, path, MODE_STRING(self->flags));
                break;
            case 1:
                sydbox_access_violation(child->pid, path, "%s(?, \"%s\", %s)",
                                        sname, path, MODE_STRING(self->flags));
                break;
            case 2:
                sydbox_access_violation(child->pid, path, "%s(?, ?, \"%s\", %s)",
                                        sname, path, MODE_STRING(self->flags));
                break;
            case 3:
                sydbox_access_violation(child->pid, path, "%s(?, ?, ?, \"%s\", %s)",
                                        sname, path, MODE_STRING(self->flags));
                break;
            default:
                g_assert_not_reached ();
                break;
        }
        data->result = RS_DENY;
    }
}

static bool systemcall_check_network_whitelist(struct checkdata *data)
{
    GSList *walk;
    struct sydbox_addr *addr;

    walk = sydbox_config_get_network_whitelist();
    while (NULL != walk) {
        addr = (struct sydbox_addr *) walk->data;
        g_debug("Checking whitelisted address {family=%d addr=%s port=%d} for equality",
                addr->family, addr->addr, addr->port);
        if (data->family == addr->family && data->port == addr->port &&
                0 == strncmp(data->addr, addr->addr, strlen(addr->addr) + 1)) {
            g_debug("Whitelisted connection {family:%d addr:%s port:%d}", addr->family, addr->addr, addr->port);
            return true;
        }
        walk = g_slist_next(walk);
    }
    return false;
}


static void systemcall_check(SystemCall *self, gpointer ctx_ptr,
                             gpointer child_ptr, gpointer data_ptr)
{
    context_t *ctx = (context_t *) ctx_ptr;
    struct tchild *child = (struct tchild *) child_ptr;
    struct checkdata *data = (struct checkdata *) data_ptr;

    if (G_UNLIKELY(RS_ALLOW != data->result))
        return;

    if (child->sandbox->network &&
            child->sandbox->network_mode != SYDBOX_NETWORK_ALLOW &&
            IS_NET_CALL(self->flags) && IS_SUPPORTED_FAMILY(data->family)) {
        bool violation;

        violation = false;
        if (child->sandbox->network_mode == SYDBOX_NETWORK_DENY) {
            g_debug("net.default is deny, checking if the connection is whitelisted");
            violation = !systemcall_check_network_whitelist(data);
        }
        else if (child->sandbox->network_mode == SYDBOX_NETWORK_LOCAL) {
            if (child->sandbox->network_restrict_connect &&
                    (NET_RESTRICTED_CALL(self->flags) ||
                     (self->flags & DECODE_SOCKETCALL && NET_RESTRICTED_SUBCALL(data->socket_subcall)))) {
                g_debug("net.restrict_connect is set, checking if connect/sendto call is whitelisted");
                violation = !systemcall_check_network_whitelist(data);
            }
            else if (data->family != AF_UNIX && !net_localhost(data->addr))
                violation = true;
        }
        else
            g_assert_not_reached();

        if (violation) {
            switch (data->family) {
                case AF_UNIX:
                    sydbox_access_violation(child->pid, NULL, "%s{family=AF_UNIX path=%s}", sname, data->addr);
                    break;
                case AF_INET:
                    sydbox_access_violation(child->pid, NULL, "%s{family=AF_INET addr=%s port=%d}",
                            sname, data->addr, data->port);
                    break;
                case AF_INET6:
                    sydbox_access_violation(child->pid, NULL, "%s{family=AF_INET6 addr=%s port=%d}",
                            sname, data->addr, data->port);
                    break;
                default:
                    g_assert_not_reached();
            }
            data->result = RS_DENY;
            child->retval = -ECONNREFUSED;
        }
        return;
    }

    if (!ctx->before_initial_execve && child->sandbox->exec && self->flags & EXEC_CALL) {
        g_debug("checking `%s' for exec access", data->rpathlist[0]);
        int allow_exec = pathlist_check(child->sandbox->exec_prefixes, data->rpathlist[0]);
        if (!allow_exec) {
            sydbox_access_violation(child->pid, data->rpathlist[0],
                    "execve(\"%s\", argv[], envp[])", data->rpathlist[0]);
            data->result = RS_DENY;
            child->retval = -EACCES;
        }
        return;
    }

    if (!child->sandbox->path)
        return;
    if (self->flags & CHECK_PATH) {
        systemcall_check_path(self, child, 0, data);
        if (RS_ERROR == data->result || RS_DENY == data->result)
            return;
    }
    if (self->flags & CHECK_PATH2) {
        systemcall_check_path(self, child, 1, data);
        if (RS_ERROR == data->result || RS_DENY == data->result)
            return;
    }
    if (self->flags & CHECK_PATH_AT) {
        systemcall_check_path(self, child, 1, data);
        if (RS_ERROR == data->result || RS_DENY == data->result)
            return;
    }
    if (self->flags & CHECK_PATH_AT1) {
        systemcall_check_path(self, child, 2, data);
        if (RS_ERROR == data->result || RS_DENY == data->result)
            return;
    }
    if (self->flags & CHECK_PATH_AT2) {
        systemcall_check_path(self, child, 3, data);
        if (RS_ERROR == data->result || RS_DENY == data->result)
            return;
    }
}

static void systemcall_end_check(SystemCall *self, gpointer ctx_ptr,
                                 gpointer child_ptr, gpointer data_ptr)
{
    context_t *ctx = (context_t *) ctx_ptr;
    struct tchild *child = (struct tchild *) child_ptr;
    struct checkdata *data = (struct checkdata *) data_ptr;

    g_debug("ending check for system call %d(%s), child %i", self->no, sname, child->pid);

    if (ctx->before_initial_execve && self->flags & EXEC_CALL) {
        g_debug("setting before_initial_execve flag to off");
        ctx->before_initial_execve = false;
    }

    for (unsigned int i = 0; i < 2; i++)
        g_free(data->dirfdlist[i]);
    for (unsigned int i = 0; i < 4; i++) {
        g_free(data->pathlist[i]);
        g_free(data->rpathlist[i]);
    }

    if (data->addr != NULL)
        g_free(data->addr);
}

static void systemcall_class_init(SystemCallClass *cls)
{
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


GType systemcall_get_type(void)
{
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

void syscall_init(void)
{
    static bool initialized = false;
    if (initialized)
        return;

    g_type_init();

    SystemCallHandler = g_object_new(TYPE_SYSTEMCALL,
            "no",       -1,
            "flags",    -1,
            NULL);
    g_signal_connect(SystemCallHandler, "check", (GCallback) systemcall_flags, NULL);
    g_signal_connect(SystemCallHandler, "check", (GCallback) systemcall_magic, NULL);
    g_signal_connect(SystemCallHandler, "check", (GCallback) systemcall_resolve, NULL);
    g_signal_connect(SystemCallHandler, "check", (GCallback) systemcall_canonicalize, NULL);
    g_signal_connect(SystemCallHandler, "check", (GCallback) systemcall_check, NULL);
    g_signal_connect(SystemCallHandler, "check", (GCallback) systemcall_end_check, NULL);

    initialized = true;
}

void syscall_free(void)
{
    g_object_unref(SystemCallHandler);
    SystemCallHandler = NULL;
}

/* Lookup a handler for the system call.
 * Return the handler if found, NULL otherwise.
 */
SystemCall *syscall_get_handler(int personality, int no)
{
    int flags;

    flags = dispatch_lookup(personality, no);
    if (-1 == flags)
        return NULL;
    SystemCallHandler->no = no;
    SystemCallHandler->flags = flags;
    return SystemCallHandler;
}

/* BAD_SYSCALL handler for system calls.
 * This function restores real call number for the denied system call and sets
 * return code.
 * Returns nonzero if child is dead, zero otherwise.
 */
static int syscall_handle_badcall(struct tchild *child)
{
    g_debug("restoring real call number for denied system call %lu(%s)", child->sno, sname);
    // Restore real call number and return our error code
    if (0 > trace_set_syscall(child->pid, child->sno)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            /* Error setting system call using ptrace()
             * child is still alive, hence the error is fatal.
             */
            g_critical("failed to restore system call: %s", g_strerror(errno));
            g_printerr("failed to restore system call: %s", g_strerror(errno));
            exit(-1);
        }
        // Child is dead.
        return -1;
    }
    if (0 > trace_set_return(child->pid, child->retval)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            /* Error setting return code using ptrace()
             * child is still alive, hence the error is fatal.
             */
            g_critical("failed to set return code: %s", g_strerror(errno));
            g_printerr("failed to set return code: %s", g_strerror(errno));
            exit(-1);
        }
        // Child is dead.
        return -1;
    }
    return 0;
}

/* chdir(2) handler for system calls.
 * This is only called when child is exiting chdir() or fchdir() system calls.
 * Returns nonzero if child is dead, zero otherwise.
 */
static int syscall_handle_chdir(struct tchild *child)
{
    long retval;

    if (0 > trace_get_return(child->pid, &retval)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            /* Error getting return code using ptrace()
             * child is still alive, hence the error is fatal.
             */
            g_critical("failed to get return code: %s", g_strerror(errno));
            g_printerr("failed to get return code: %s", g_strerror(errno));
            exit(-1);
        }
        // Child is dead.
        return -1;
    }
    if (0 == retval) {
        /* Child has successfully changed directory,
         * update current working directory.
         */
        char *newcwd = pgetcwd(child->pid);
        if (NULL == newcwd) {
            /* Failed to get current working directory of child.
             * Set errno of the child.
             * FIXME: We should probably die here as well, because the
             * child has successfully changed directory and setting
             * errno doesn't change that fact.
             */
            retval = -errno;
            g_debug("pgetcwd() failed: %s", g_strerror(errno));
            if (0 > trace_set_return(child->pid, retval)) {
                if (G_UNLIKELY(ESRCH != errno)) {
                    /* Error setting return code using ptrace()
                     * child is still alive, hence the error is fatal.
                     */
                    g_critical("failed to set return code: %s", g_strerror(errno));
                    g_printerr("failed to set return code: %s", g_strerror(errno));
                    exit(-1);
                }
                // Child is dead.
                return -1;
            }
        }
        else {
            /* Successfully determined the new current working
             * directory of child. Update context.
             */
            if (NULL != child->cwd)
                g_free(child->cwd);
            child->cwd = newcwd;
            g_info("child %i has changed directory to '%s'", child->pid, child->cwd);
        }
    }
    else {
        /* Child failed to change current working directory,
         * nothing to do.
         */
        g_debug("child %i failed to change directory: %s", child->pid, g_strerror(-retval));
    }
    return 0;
}

/**
 * bind(2) handler
 */
static int syscall_handle_bind(struct tchild *child, int flags)
{
    int subcall, family, port;
    long retval;
    char *addr;
    GSList *whitelist;

    if (0 > trace_get_return(child->pid, &retval)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            /* Error getting return code using ptrace()
             * child is still alive, hence the error is fatal.
             */
            g_critical("failed to get return code: %s", g_strerror(errno));
            g_printerr("failed to get return code: %s", g_strerror(errno));
            exit(-1);
        }
        // Child is dead.
        return -1;
    }

    if (0 != retval) {
        /* Bind call failed, ignore it */
        return 0;
    }

    if (flags & DECODE_SOCKETCALL) {
        subcall = trace_decode_socketcall(child->pid, child->personality);
        if (0 > subcall) {
            if (G_UNLIKELY(ESRCH != errno)) {
                /* Error getting socket subcall using ptrace()
                 * child is still alive, hence the error is fatal.
                 */
                g_critical("Failed to decode socketcall: %s", g_strerror(errno));
                g_printerr("Failed to decode socketcall: %s", g_strerror(errno));
                exit(-1);
            }
            // Child is dead.
            return -1;
        }
        if (subcall != SOCKET_SUBCALL_BIND)
            return 0;

        addr = trace_get_addr(child->pid, child->personality, 1, true, &family, &port);
    }
    else if (flags & BIND_CALL)
        addr = trace_get_addr(child->pid, child->personality, 1, false, &family, &port);
    else
        g_assert_not_reached();

    if (NULL == addr) {
        if (G_UNLIKELY(ESRCH != errno)) {
            /* Error getting address using ptrace()
             * child is still alive, hence the error is fatal.
             */
            g_critical("Failed to get address of bind() call: %s", g_strerror(errno));
            g_printerr("Failed to get address of bind() call: %s", g_strerror(errno));
            exit(-1);
        }
        // Child is dead
        return -1;
    }

    if (IS_SUPPORTED_FAMILY(family)) {
        g_debug("Whitelisting successful bind() addr:%s port:%d", addr, port);
        whitelist = sydbox_config_get_network_whitelist();
        netlist_new(&whitelist, family, port, addr);
        sydbox_config_set_network_whitelist(whitelist);
    }
    g_free(addr);
    return 0;
}

#if defined(POWERPC)
/* clone(2) handler for POWERPC because PTRACE_GETEVENTMSG doesn't work
 * reliably on this architecture.
 */
static int syscall_handle_clone(context_t *ctx, struct tchild *child)
{
    long retval;
    struct tchild *newchild;

    if (0 > trace_get_return(child->pid, &retval)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            /* Error getting return code using ptrace()
             * child is still alive, hence the error is fatal.
             */
            g_critical("failed to get return code: %s", g_strerror(errno));
            g_printerr("failed to get return code: %s", g_strerror(errno));
            exit(-1);
        }
        // Child is dead.
        return -1;
    }

    if (retval < 0) {
        /* clone() failed, no child is born. */
        return 0;
    }

    newchild = tchild_find(ctx->children, retval);
    if (NULL != newchild) {
        if (newchild & TCHILD_NEEDINHERIT)
            tchild_inherit(newchild, child);
    }
    else {
        tchild_new(ctx->children, retval);
        newchild = tchild_find(ctx->children, retval);
        tchild_inherit(newchild, child);
    }

    if (0 > trace_syscall(newchild->pid, 0)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            g_critical("failed to resume child %i: %s", child->pid, g_strerror (errno));
            g_printerr("failed to resume child %i: %s", child->pid, g_strerror (errno));
            exit(-1);
        }
        context_remove_child(ctx, newchild->pid);
    }
    return 0;
}
#endif // defined(POWERPC)

/* Main syscall handler
 */
int syscall_handle(context_t *ctx, struct tchild *child)
{
    int flags;
    long sno;
    struct checkdata data;
    SystemCall *handler;

    if (!(child->flags & TCHILD_INSYSCALL)) {
        /* Child is entering the system call.
         * Get the system call number of child.
         * Save it in child->sno.
         */
        if (0 > trace_get_syscall(child->pid, &sno)) {
            if (G_UNLIKELY(ESRCH != errno)) {
                /* Error getting system call using ptrace()
                 * child is still alive, hence the error is fatal.
                 */
                g_critical("failed to get system call: %s", g_strerror(errno));
                g_printerr("failed to get system call: %s", g_strerror(errno));
                exit(-1);
            }
            // Child is dead, remove it
            return context_remove_child(ctx, child->pid);
        }
        child->sno = sno;
        sname = dispatch_name(child->personality, child->sno);
    }
    else
        sno = child->sno;

    if (!(child->flags & TCHILD_INSYSCALL)) { // Entering syscall
        g_debug_trace("child %i is entering system call %lu(%s)", child->pid, sno, sname);

        /* Get handler for the system call
         */
        handler = syscall_get_handler(child->personality, sno);
        if (NULL == handler) {
            /* There's no handler for this system call.
             * Safe system call, allow access.
             */
            g_debug_trace("allowing access to system call %lu(%s)", sno, sname);
        }
        else {
            /* There's a handler for this system call,
             * call the handler.
             */
            memset(&data, 0, sizeof(struct checkdata));
            g_signal_emit_by_name(handler, "check", ctx, child, &data);

            /* Check result */
            switch(data.result) {
                case RS_ERROR:
                    if (ESRCH == errno)
                        return context_remove_child(ctx, child->pid);
                    else if (EIO != errno && EFAULT != errno) {
                        g_critical("error while checking system call %lu(%s) for access: %s",
                                sno, sname, g_strerror(errno));
                        g_printerr("error while checking system call %lu(%s) for access: %s",
                                sno, sname, g_strerror(errno));
                        exit(-1);
                    }
                    /* fall through */
                case RS_DENY:
                    g_debug("denying access to system call %lu(%s)", sno, sname);
                    child->flags |= TCHILD_DENYSYSCALL;
                    if (0 > trace_set_syscall(child->pid, BAD_SYSCALL)) {
                        if (G_UNLIKELY(ESRCH != errno)) {
                            g_critical("failed to set system call: %s", g_strerror(errno));
                            g_printerr("failed to set system call: %s", g_strerror(errno));
                            exit(-1);
                        }
                        return context_remove_child(ctx, child->pid);
                    }
                    break;
                case RS_ALLOW:
                case RS_NOWRITE:
                case RS_MAGIC:
                    g_debug_trace("allowing access to system call %lu(%s)", sno, sname);
                    break;
                default:
                    g_assert_not_reached();
                    break;
            }
        }
    }
    else { // Exiting sytem call
        g_debug_trace("child %i is exiting system call %lu(%s)", child->pid, sno, sname);

        if (child->flags & TCHILD_DENYSYSCALL) {
            /* Child is exiting a denied system call.
             */
            if (0 > syscall_handle_badcall(child))
                return context_remove_child(ctx, child->pid);
            child->flags &= ~TCHILD_DENYSYSCALL;
        }
        else if (dispatch_chdir(child->personality, sno)) {
            /* Child is exiting a system call that may have changed its current
             * working directory. Update current working directory.
             */
            if (0 > syscall_handle_chdir(child))
                return context_remove_child(ctx, child->pid);
        }
        else if (child->sandbox->network_restrict_connect && dispatch_maybind(child->personality, sno)) {
            flags = dispatch_lookup(child->personality, sno);
            if (0 > syscall_handle_bind(child, flags))
                return context_remove_child(ctx, child->pid);
        }
#if defined(POWERPC)
        else if (dispatch_clone(child->personality, sno)) {
            if (0 > syscall_handle_clone(ctx, child))
                return context_remove_child(ctx, child->pid);
        }
#endif // defined(POWERPC)
    }
    child->flags ^= TCHILD_INSYSCALL;
    return 0;
}

