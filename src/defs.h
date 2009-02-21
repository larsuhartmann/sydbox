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

/* Per process tracking data */
struct tchild {
    /* process id of the traced child */
    pid_t pid;
    /* we will get a stop signal from child and will setup tracing flags */
    int need_setup;
    /* child is in syscall */
    int in_syscall;
    /* original syscall number when a syscall is faked */
    unsigned long orig_syscall;
    /* faked syscall will fail with this error code */
    int error_code;
    struct tchild *next;
};

void tchild_new(struct tchild **head, pid_t pid);
void tchild_free(struct tchild **head);
void tchild_delete(struct tchild **head, pid_t pid);
struct tchild *tchild_find(struct tchild **head, pid_t pid);
void tchild_setup(struct tchild *child);
unsigned int tchild_event(struct tchild *child, int status);

/* context.c */
typedef struct {
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
void _die(int err, const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 2, 3)));
void lg(int level, const char *id, const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 3, 4)));
void access_error(pid_t pid, const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 2, 3)));

void *xmalloc(size_t size);
char *xstrndup(const char *s, size_t n);

int remove_slash(const char *pathname, char *dest);
char *resolve_path(const char *path, pid_t pid, int resolve, int *issymlink);
void bash_expand(const char *pathname, char *dest);

/* trace.c */
#define ADDR_MUL        ((64 == __WORDSIZE) ? 8 : 4)
#define MAX_ARGS        4
#if defined(I386)
#define ORIG_ACCUM      (4 * ORIG_EAX)
#define ACCUM           (4 * EAX)
static const int syscall_args[MAX_ARGS] = {4 * EBX, 4 * ECX, 4 * EDX, 4 * ESI};
#elif defined(X86_64)
#define ORIG_ACCUM      (8 * ORIG_RAX)
#define ACCUM           (8 * RAX)
static const int syscall_args[MAX_ARGS] = {8 * RDI, 8 * RSI, 8 * RDX, 8 * R10};
#endif

int ptrace_get_syscall(pid_t pid);
void ptrace_set_syscall(pid_t pid, int syscall);
void ptrace_get_string(pid_t pid, int arg, char *dest, size_t len);
void ptrace_set_string(pid_t pid, int arg, char *src, size_t len);

/* syscall.c */
/* Possible results for a syscall */
enum result {
    R_DENY_VIOLATION, /* Deny the system call and raise an access violation. */
    R_DENY_RETURN, /* Deny the system call and make it return the specified return code. */
    R_ALLOW, /* Allow the system call to be called */
    R_NONMAGIC /* Internal used by syscall_check_magic() */
};

#define REASON_MAX (PATH_MAX + 128)
struct decision {
    enum result res;
    int ret;
    char reason[REASON_MAX];
};

#define CMD_PATH                "/dev/sydbox/"
#define CMD_PATH_LEN            12
#define CMD_WRITE               CMD_PATH"write/"
#define CMD_WRITE_LEN           (CMD_PATH_LEN + 6)
#define CMD_PREDICT             CMD_PATH"predict/"
#define CMD_PREDICT_LEN         (CMD_PATH_LEN + 8)

int syscall_check_path(context_t *ctx, struct tchild *child,
        struct decision *decs, int arg, int sflags, const char *sname);
struct decision syscall_check(context_t *ctx, struct tchild *child, int syscall);
int syscall_handle(context_t *ctx, struct tchild *child);

#endif /* SYDBOX_GUARD_DEFS_H */
