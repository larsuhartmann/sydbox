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

/* pink floyd */
#define PINK_FLOYD  "       ..uu.\n" \
                    "       ?$\"\"`?i           z'\n" \
                    "       `M  .@\"          x\"\n" \
                    "       'Z :#\"  .   .    f 8M\n" \
                    "       '&H?`  :$f U8   <  MP   x#'\n" \
                    "       d#`    XM  $5.  $  M' xM\"\n" \
                    "     .!\">     @  'f`$L:M  R.@!`\n" \
                    "    +`  >     R  X  \"NXF  R\"*L\n" \
                    "        k    'f  M   \"$$ :E  5.\n" \
                    "        %%    `~  \"    `  'K  'M\n" \
                    "            .uH          'E   `h\n" \
                    "         .x*`             X     `\n" \
                    "      .uf`                *\n" \
                    "    .@8     .\n" \
                    "   'E9F  uf\"          ,     ,\n" \
                    "     9h+\"   $M    eH. 8b. .8    .....\n" \
                    "    .8`     $'   M 'E  `R;'   d?\"\"\"`\"#\n" \
                    "   ` E      @    b  d   9R    ?*     @\n" \
                    "     >      K.zM `%%M'   9'    Xf   .f\n" \
                    "    ;       R'          9     M  .=`\n" \
                    "    t                   M     Mx~\n" \
                    "    @                  lR    z\"\n" \
                    "    @                  `   ;\"\n" \
                    "                          `\n"

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
    char *path;
    struct pathnode *next;
};

#define CMD_PATH                "/dev/sydbox/"
#define CMD_PATH_LEN            12
#define CMD_WRITE               CMD_PATH"write/"
#define CMD_WRITE_LEN           (CMD_PATH_LEN + 6)
#define CMD_PREDICT             CMD_PATH"predict/"
#define CMD_PREDICT_LEN         (CMD_PATH_LEN + 8)
#define CMD_RMWRITE             CMD_PATH"unwrite/"
#define CMD_RMWRITE_LEN         (CMD_PATH_LEN + 8)
#define CMD_RMPREDICT           CMD_PATH"unpredict/"
#define CMD_RMPREDICT_LEN       (CMD_PATH_LEN + 10)

int path_magic_dir(const char *path);
int path_magic_write(const char *path);
int path_magic_predict(const char *path);
int path_magic_rmwrite(const char *path);
int path_magic_rmpredict(const char *path);
int pathnode_new(struct pathnode **head, const char *path);
void pathnode_free(struct pathnode **head);
void pathnode_delete(struct pathnode **head, const char *path);
int pathlist_init(struct pathnode **pathlist, const char *pathlist_env);
int pathlist_check(struct pathnode **pathlist, const char *path);

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
    int hasmagic; /* Whether the child is allowed to execute magic commands */
    int exec_count; /* Allow this number of execve calls to bypass magic call disallow check */
    char *cwd; /* child's current working directory */
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

extern context_t *ctx;

/* canonicalize.c */
char *erealpath(const char *name, char *resolved);

/* util.c */
extern int log_level;
extern char log_file[PATH_MAX];
extern FILE *log_fp;
extern int colour;

#define LOG_ERROR       1
#define LOG_WARNING     2
#define LOG_NORMAL      3
#define LOG_VERBOSE     4
#define LOG_DEBUG       5
#define LOG_DEBUG_CRAZY 6

#define NORMAL  "[00;00m"
#define MAGENTA "[00;35m"
#define PINK    "[01;35m"

void die(int err, const char *fmt, ...)
    __attribute__ ((noreturn))
    __attribute__ ((format (printf, 2, 3)));
#define DIESOFT(...)    die(EX_SOFTWARE, __VA_ARGS__)
#define DIEDATA(...)    die(EX_DATAERR, __VA_ARGS__)
#define DIEOS(...)      die(EX_OSERR, __VA_ARGS__)
#define DIEUSER(...)    die(EX_USAGE, __VA_ARGS__)
void _die(int err, const char *fmt, ...)
    __attribute__ ((noreturn))
    __attribute__ ((format (printf, 2, 3)));
void access_error(pid_t pid, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));

void lg(int level, const char *func, const char *fmt, ...)
    __attribute__ ((format (printf, 3, 4)));
#define LOGE(...)   lg(LOG_ERROR, __func__, __VA_ARGS__)
#define LOGW(...)   lg(LOG_WARNING, __func__, __VA_ARGS__)
#define LOGN(...)   lg(LOG_NORMAL, __func__, __VA_ARGS__)
#define LOGV(...)   lg(LOG_VERBOSE, __func__, __VA_ARGS__)
#define LOGD(...)   lg(LOG_DEBUG, __func__, __VA_ARGS__)
#define LOGC(...)   lg(LOG_DEBUG_CRAZY, __func__, __VA_ARGS__)

void *__xmalloc(size_t size, const char *file, const char *func, size_t line)
    __attribute__ ((alloc_size(1)));
void *__xrealloc(void *ptr, size_t size, const char *file, const char *func, size_t line)
    __attribute__ ((alloc_size(2)));
char *__xstrndup(const char *str, size_t size, const char *file, const char *func, size_t line)
    __attribute__ ((alloc_size(2)))
    __attribute__ ((nonnull(1)));
#define xmalloc(_size)          __xmalloc(_size, __FILE__, __func__, __LINE__)
#define xrealloc(_ptr, _size)   __xrealloc(_ptr, _size, __FILE__, __func__, __LINE__)
#define xstrndup(_str, _size)   __xstrndup(_str, _size, __FILE__, __func__, __LINE__)

int remove_slash(char *dest, const char *src);
void shell_expand(char *dest, const char *src);
char *getcwd_pid(char *dest, size_t size, pid_t pid);
char *resolve_path(const char *path, int resolve);

int handle_esrch(context_t *ctx, struct tchild *child);
int can_exec(const char *file);

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
int trace_get_return(pid_t pid, long *res);
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
