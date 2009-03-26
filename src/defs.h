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

/* context.c */
typedef struct {
    int paranoid;
    char *cwd; // current working directory
    struct tchild *children;
    struct tchild *eldest;  // first child is kept to determine return code
} context_t;

context_t *context_new(void);
void context_free(context_t *ctx);

/* getcwd.c */
char *egetcwd(void);
int echdir(char *dir);

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

#endif /* SYDBOX_GUARD_DEFS_H */
