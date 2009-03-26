/**
 * Copyright 2009 Saleem Abdulrasool <compnerd@compnerd.org>
 **/

#ifndef __UTIL_H__
#define __UTIL_H__

#include "context.h"
#include "children.h"

#define NORMAL  "[00;00m"
#define MAGENTA "[00;35m"
#define PINK    "[01;35m"

#define DIESOFT(...)    die(EX_SOFTWARE, __VA_ARGS__)
#define DIEDATA(...)    die(EX_DATAERR, __VA_ARGS__)
#define DIEOS(...)      die(EX_OSERR, __VA_ARGS__)
#define DIEUSER(...)    die(EX_USAGE, __VA_ARGS__)

#define xmalloc(_size)          __xmalloc(_size, __FILE__, __func__, __LINE__)
#define xcalloc(_nmemb, _size)  __xcalloc(_nmemb, _size, __FILE__, __func__, __LINE__)
#define xrealloc(_ptr, _size)   __xrealloc(_ptr, _size, __FILE__, __func__, __LINE__)
#define xstrndup(_str, _size)   __xstrndup(_str, _size, __FILE__, __func__, __LINE__)
#define xstrdup(_str)           __xstrndup(_str, strlen(_str) + 1, __FILE__, __func__, __LINE__)

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

