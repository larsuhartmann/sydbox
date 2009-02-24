/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel
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

#ifndef SYDBOX_GUARD_DEFS_H
#define SYDBOX_GUARD_DEFS_H 1

#include <limits.h>
#include <sysexits.h>
#include <sys/types.h>
#include <stdio.h> /* FILE */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif /* HAVE_SYS_REG_H */

/* environment */
#define ENV_PHASE       "SANDBOX_PHASE"
#define ENV_LOG         "SANDBOX_LOG"
#define ENV_CONFIG      "SANDBOX_CONFIG"
#define ENV_WRITE       "SANDBOX_WRITE"
#define ENV_PREDICT     "SANDBOX_PREDICT"
#define ENV_NET         "SANDBOX_NET"
#define ENV_NO_COLOUR   "SANDBOX_NO_COLOUR"

/* path.c */
struct pathnode {
    char *pathname;
    struct pathnode *next;
};

#define CMD_PATH                "/dev/sydbox/"
#define CMD_PATH_LEN            12
#define CMD_WRITE               CMD_PATH"write/"
#define CMD_WRITE_LEN           (CMD_PATH_LEN + 6)
#define CMD_PREDICT             CMD_PATH"predict/"
#define CMD_PREDICT_LEN         (CMD_PATH_LEN + 8)

int path_magic_dir(const char *pathname);
int path_magic_write(const char *pathname);
int path_magic_predict(const char *pathname);
void pathnode_new(struct pathnode **head, const char *pathname);
void pathnode_free(struct pathnode **head);
int pathlist_init(struct pathnode **pathlist, const char *pathlist_env);
int pathlist_check(struct pathnode **pathlist, const char *pathname);

/* children.c */
/* Events */
enum {
    E_SETUP = 0,
    E_SETUP_PREMATURE,
    E_SYSCALL,
    E_FORK,
    E_EXECV,
    E_GENUINE,
    E_EXIT,
    E_EXIT_SIGNAL,
    E_UNKNOWN
};

/* TCHILD_ flags */
#define TCHILD_NEEDSETUP (1 << 0) /* Child needs setup */
#define TCHILD_INSYSCALL (1 << 1) /* Child is in syscall */

/* Per process tracking data */
struct tchild {
    int flags; /* TCHILD_ flags */
    pid_t pid;
    unsigned long syscall; /* original syscall when system call is faked */
    long retval; /* faked syscall will return this value */
    struct tchild *next;
};

void tchild_new(struct tchild **head, pid_t pid);
void tchild_free(struct tchild **head);
void tchild_delete(struct tchild **head, pid_t pid);
struct tchild *tchild_find(struct tchild **head, pid_t pid);
unsigned int tchild_event(struct tchild *child, int status);

/* context.c */
typedef struct {
    int paranoid;
    int net_allowed;
    struct pathnode *write_prefixes;
    struct pathnode *predict_prefixes;
    struct tchild *children;
    /* first child pointer is kept for determining return code */
    struct tchild *eldest;
} context_t;

context_t *context_new(void);
void context_free(context_t *ctx);
int context_cmd_allowed(context_t *ctx, struct tchild *child);

/* realpath.c */
char *safe_realpath(const char *path, pid_t pid, int resolv, int *issymlink);

/* util.c */
char log_file[PATH_MAX];
FILE *flog;

#define LOG_ERROR       1
#define LOG_WARNING     2
#define LOG_NORMAL      3
#define LOG_VERBOSE     4
#define LOG_DEBUG       5
#define LOG_DEBUG_CRAZY 6

int log_level;

#define NORMAL  "[00;00m"
#define MAGENTA "[00;35m"
#define PINK    "[01;35m"
int colour;

void die(int err, const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 2, 3)));
#define DIESOFT(...)    die(EX_SOFTWARE, __VA_ARGS__)
#define DIEDATA(...)    die(EX_DATAERR, __VA_ARGS__)
#define DIEOS(...)      die(EX_OSERR, __VA_ARGS__)
#define DIEUSER(...)    die(EX_USAGE, __VA_ARGS__)
void _die(int err, const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 2, 3)));
void access_error(pid_t pid, const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 2, 3)));

void lg(int level, const char *funcname, const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 3, 4)));
#define LOGE(...)   lg(LOG_ERROR, __func__, __VA_ARGS__)
#define LOGW(...)   lg(LOG_WARNING, __func__, __VA_ARGS__)
#define LOGN(...)   lg(LOG_NORMAL, __func__, __VA_ARGS__)
#define LOGV(...)   lg(LOG_VERBOSE, __func__, __VA_ARGS__)
#define LOGD(...)   lg(LOG_DEBUG, __func__, __VA_ARGS__)
#define LOGC(...)   lg(LOG_DEBUG_CRAZY, __func__, __VA_ARGS__)

void *xmalloc(size_t size);
char *xstrndup(const char *s, size_t n);

int remove_slash(const char *pathname, char *dest);
void shell_expand(const char *pathname, char *dest);
char *resolve_path(const char *path, pid_t pid, int resolve, int *issymlink);

int handle_esrch(context_t *ctx, struct tchild *child);
/* trace.c */
int trace_me(void);
int trace_setup(pid_t pid);
int trace_kill(pid_t pid);
int trace_cont(pid_t pid);
int trace_syscall(pid_t pid, int data);
int trace_geteventmsg(pid_t pid, void *data);
int trace_get_arg(pid_t pid, int arg, long *res);
int trace_get_syscall(pid_t pid, long *syscall);
int trace_set_syscall(pid_t pid, long syscall);
int trace_set_return(pid_t pid, long val);
int trace_get_string(pid_t pid, int arg, char *dest, size_t len);
int trace_set_string(pid_t pid, int arg, const char *src, size_t len);

/* syscall.c */
struct syscall_def {
    int no;
    unsigned int flags;
};

enum res_syscall {
    RS_DENY, // Deny access
    RS_ALLOW, // Allow access
    RS_NONMAGIC, // open() or stat() not magic (internal)
    RS_ERROR = EX_SOFTWARE // An error occured while checking access
};

enum res_syscall syscall_check(context_t *ctx, struct tchild *child, int syscall);
int syscall_handle(context_t *ctx, struct tchild *child);

#endif /* SYDBOX_GUARD_DEFS_H */
