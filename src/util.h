/**
 * Copyright 2009 Saleem Abdulrasool <compnerd@compnerd.org>
 **/

#ifndef __UTIL_H__
#define __UTIL_H__

#include "defs.h"
#include "children.h"

#define LOG_ERROR       1
#define LOG_WARNING     2
#define LOG_NORMAL      3
#define LOG_VERBOSE     4
#define LOG_DEBUG       5
#define LOG_DEBUG_CRAZY 6

#define NORMAL  "[00;00m"
#define MAGENTA "[00;35m"
#define PINK    "[01;35m"

#define LOGE(...)   lg(LOG_ERROR, __func__, __LINE__,  __VA_ARGS__)
#define LOGW(...)   lg(LOG_WARNING, __func__, __LINE__, __VA_ARGS__)
#define LOGN(...)   lg(LOG_NORMAL, __func__, __LINE__, __VA_ARGS__)
#define LOGV(...)   lg(LOG_VERBOSE, __func__, __LINE__, __VA_ARGS__)
#define LOGD(...)   lg(LOG_DEBUG, __func__, __LINE__, __VA_ARGS__)
#define LOGC(...)   lg(LOG_DEBUG_CRAZY, __func__, __LINE__, __VA_ARGS__)

#define DIESOFT(...)    die(EX_SOFTWARE, __VA_ARGS__)
#define DIEDATA(...)    die(EX_DATAERR, __VA_ARGS__)
#define DIEOS(...)      die(EX_OSERR, __VA_ARGS__)
#define DIEUSER(...)    die(EX_USAGE, __VA_ARGS__)

#define xmalloc(_size)          __xmalloc(_size, __FILE__, __func__, __LINE__)
#define xcalloc(_nmemb, _size)  __xcalloc(_nmemb, _size, __FILE__, __func__, __LINE__)
#define xrealloc(_ptr, _size)   __xrealloc(_ptr, _size, __FILE__, __func__, __LINE__)
#define xstrndup(_str, _size)   __xstrndup(_str, _size, __FILE__, __func__, __LINE__)
#define xstrdup(_str)           __xstrndup(_str, strlen(_str) + 1, __FILE__, __func__, __LINE__)

extern int log_level;
extern char *log_file;
extern FILE *log_fp;
extern int colour;

void
die (int err, const char *fmt, ...)
    __attribute__ ((noreturn))
    __attribute__ ((format (printf, 2, 3)));

void
_die (int err, const char *fmt, ...)
    __attribute__ ((noreturn))
    __attribute__ ((format (printf, 2, 3)));

void
access_error (pid_t pid, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));

void
lg (int level, const char *func, size_t line, const char *fmt, ...)
    __attribute__ ((format (printf, 4, 5)));

void *
__xmalloc (size_t size, const char *file, const char *func, size_t line)
    __attribute__ ((alloc_size(1)));

void *
__xcalloc (size_t nmemb, size_t size, const char *file, const char *func, size_t line)
    __attribute__ ((alloc_size(1,2)));

void *
__xrealloc (void *ptr, size_t size, const char *file, const char *func, size_t line)
    __attribute__ ((alloc_size(2)));

char *
__xstrndup (const char *str, size_t size, const char *file, const char *func, size_t line)
    __attribute__ ((alloc_size(2)))
    __attribute__ ((nonnull(1)));

char *
remove_slash (const char *src);

char *
shell_expand (const char *src);

int
handle_esrch (context_t *ctx, struct tchild *child);

#endif

/* vim: set sw=4 sts=4 fdm=syntax et : */

