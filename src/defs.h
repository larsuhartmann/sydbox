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
#include <stdbool.h>
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
#define ENV_PROFILE     "SANDBOX_PROFILE"
#define ENV_LOG         "SANDBOX_LOG"
#define ENV_CONFIG      "SANDBOX_CONFIG"
#define ENV_WRITE       "SANDBOX_WRITE"
#define ENV_PREDICT     "SANDBOX_PREDICT"
#define ENV_NET         "SANDBOX_NET"
#define ENV_NO_COLOUR   "SANDBOX_NO_COLOUR"

/* likely/unlikely */
#if defined(__GNUC__)
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

/* path.c */
struct pathnode {
    char *path;
    struct pathnode *next;
};

#define CMD_PATH                "/dev/sydbox/"
#define CMD_PATH_LEN            12
#define CMD_ON                  CMD_PATH"on"
#define CMD_ON_LEN              (CMD_PATH_LEN + 3)
#define CMD_OFF                 CMD_PATH"off"
#define CMD_OFF_LEN             (CMD_PATH_LEN + 4)
#define CMD_TOGGLE              CMD_PATH"toggle"
#define CMD_TOGGLE_LEN          (CMD_PATH_LEN + 7)
#define CMD_LOCK                CMD_PATH"lock"
#define CMD_LOCK_LEN            (CMD_PATH_LEN + 5)
#define CMD_EXEC_LOCK           CMD_PATH"exec_lock"
#define CMD_EXEC_LOCK_LEN       (CMD_PATH_LEN + 10)
#define CMD_WRITE               CMD_PATH"write/"
#define CMD_WRITE_LEN           (CMD_PATH_LEN + 6)
#define CMD_PREDICT             CMD_PATH"predict/"
#define CMD_PREDICT_LEN         (CMD_PATH_LEN + 8)
#define CMD_RMWRITE             CMD_PATH"unwrite/"
#define CMD_RMWRITE_LEN         (CMD_PATH_LEN + 8)
#define CMD_RMPREDICT           CMD_PATH"unpredict/"
#define CMD_RMPREDICT_LEN       (CMD_PATH_LEN + 10)

int path_magic_dir(const char *path);
int path_magic_on(const char *path);
int path_magic_off(const char *path);
int path_magic_toggle(const char *path);
int path_magic_lock(const char *path);
int path_magic_exec_lock(const char *path);
int path_magic_write(const char *path);
int path_magic_predict(const char *path);
int path_magic_rmwrite(const char *path);
int path_magic_rmpredict(const char *path);
int pathnode_new(struct pathnode **head, const char *path, int sanitize);
void pathnode_free(struct pathnode **head);
void pathnode_delete(struct pathnode **head, const char *path_sanitized);
int pathlist_init(struct pathnode **pathlist, const char *pathlist_env);
int pathlist_check(struct pathnode **pathlist, const char *path_sanitized);

/* children.c */
/* TCHILD_ flags */
#define TCHILD_NEEDSETUP (1 << 0) /* Child needs setup */
#define TCHILD_INSYSCALL (1 << 1) /* Child is in syscall */

/* Per process tracking data */
enum lock_status {
    LOCK_SET, /* Magic commands are locked */
    LOCK_UNSET, /* Magic commands are unlocked */
    LOCK_PENDING /* Magic commands will be locked when an execve() is encountered */
};

struct tdata {
    int on; /* Whether sandbox is on for the child */
    int lock; /* Whether magic commands are locked for the child */
    int net; /* Whether child is allowed to access network */
    struct pathnode *write_prefixes;
    struct pathnode *predict_prefixes;
};

struct tchild {
    int flags; /* TCHILD_ flags */
    pid_t pid;
    char *cwd; /* child's current working directory */
    unsigned long sno; /* original syscall no when a system call is faked */
    long retval; /* faked syscall will return this value */
    struct tdata *sandbox;
    struct tchild *next;
};

#ifndef PID_MAX_LIMIT
#if __WORDSIZE == 64
#define PID_MAX_LIMIT   (1 << 22)
#elif __WORDSIZE == 32
#define PID_MAX_LIMIT   (1 << 15)
#else
#error unsupported wordsize
#endif
#endif /* PID_MAX_LIMIT */
extern struct tchild *childtab[PID_MAX_LIMIT];

void tchild_new(struct tchild **head, pid_t pid);
void tchild_free(struct tchild **head);
void tchild_delete(struct tchild **head, pid_t pid);

/* context.c */
typedef struct {
    int paranoid;
    struct tchild *children;
    struct tchild *eldest;  // first child is kept to determine return code
} context_t;

context_t *context_new(void);
void context_free(context_t *ctx);

/* getcwd.c */
char *egetcwd(void);
int echdir(char *dir);

/* wrappers.c */
char *edirname(const char *path);
char *ebasename(const char *path);
char *ereadlink(const char *path);

enum canonicalize_mode_t {
    /* All components must exist.  */
    CAN_EXISTING = 0,
    /* All components excluding last one must exist.  */
    CAN_ALL_BUT_LAST = 1,
};
typedef enum canonicalize_mode_t canonicalize_mode_t;

char *canonicalize_filename_mode(const char *name, canonicalize_mode_t can_mode,
        bool resolve, const char *cwd);

/* proc.c */
char *pgetcwd(pid_t pid);
char *pgetdir(pid_t pid, int dfd);

/* util.c */
extern int log_level;
extern char *log_file;
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

void lg(int level, const char *func, size_t line, const char *fmt, ...)
    __attribute__ ((format (printf, 4, 5)));
#define LOGE(...)   lg(LOG_ERROR, __func__, __LINE__,  __VA_ARGS__)
#define LOGW(...)   lg(LOG_WARNING, __func__, __LINE__, __VA_ARGS__)
#define LOGN(...)   lg(LOG_NORMAL, __func__, __LINE__, __VA_ARGS__)
#define LOGV(...)   lg(LOG_VERBOSE, __func__, __LINE__, __VA_ARGS__)
#define LOGD(...)   lg(LOG_DEBUG, __func__, __LINE__, __VA_ARGS__)
#define LOGC(...)   lg(LOG_DEBUG_CRAZY, __func__, __LINE__, __VA_ARGS__)

void *__xmalloc(size_t size, const char *file, const char *func, size_t line)
    __attribute__ ((alloc_size(1)));
void *__xcalloc(size_t nmemb, size_t size, const char *file, const char *func, size_t line)
    __attribute__ ((alloc_size(1,2)));
void *__xrealloc(void *ptr, size_t size, const char *file, const char *func, size_t line)
    __attribute__ ((alloc_size(2)));
char *__xstrndup(const char *str, size_t size, const char *file, const char *func, size_t line)
    __attribute__ ((alloc_size(2)))
    __attribute__ ((nonnull(1)));
#define xmalloc(_size)          __xmalloc(_size, __FILE__, __func__, __LINE__)
#define xcalloc(_nmemb, _size)  __xcalloc(_nmemb, _size, __FILE__, __func__, __LINE__)
#define xrealloc(_ptr, _size)   __xrealloc(_ptr, _size, __FILE__, __func__, __LINE__)
#define xstrndup(_str, _size)   __xstrndup(_str, _size, __FILE__, __func__, __LINE__)
#define xstrdup(_str)           __xstrndup(_str, strlen(_str) + 1, __FILE__, __func__, __LINE__)

char *remove_slash(const char *src);
char *shell_expand(const char *src);

int handle_esrch(context_t *ctx, struct tchild *child);

/* trace.c */
/* Events */
enum {
    E_STOP = 0,
    E_SYSCALL,
    E_FORK,
    E_VFORK,
    E_CLONE,
    E_EXEC,
    E_GENUINE,
    E_EXIT,
    E_EXIT_SIGNAL,
    E_UNKNOWN
};
unsigned int trace_event(int status);

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
char *trace_get_string(pid_t pid, int arg);
int trace_set_string(pid_t pid, int arg, const char *src, size_t len);
int trace_fake_stat(pid_t pid);

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

enum res_syscall syscall_check(context_t *ctx, struct tchild *child, int sno);
int syscall_handle(context_t *ctx, struct tchild *child);

/* loop.c */
int trace_loop(context_t *ctx);

#endif /* SYDBOX_GUARD_DEFS_H */
