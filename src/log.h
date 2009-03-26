/**
 * Copyright 2009 Saleem Abdulrasool <compnerd@compnerd.org>
 **/

#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>

#define LOG_ERROR       1
#define LOG_WARNING     2
#define LOG_NORMAL      3
#define LOG_VERBOSE     4
#define LOG_DEBUG       5
#define LOG_DEBUG_CRAZY 6

#define LOGE(...)   lg(LOG_ERROR, __func__, __LINE__,  __VA_ARGS__)
#define LOGW(...)   lg(LOG_WARNING, __func__, __LINE__, __VA_ARGS__)
#define LOGN(...)   lg(LOG_NORMAL, __func__, __LINE__, __VA_ARGS__)
#define LOGV(...)   lg(LOG_VERBOSE, __func__, __LINE__, __VA_ARGS__)
#define LOGD(...)   lg(LOG_DEBUG, __func__, __LINE__, __VA_ARGS__)
#define LOGC(...)   lg(LOG_DEBUG_CRAZY, __func__, __LINE__, __VA_ARGS__)

extern int log_level;
extern char *log_file;
extern FILE *log_fp;

void
lg (int level, const char *func, size_t line, const char *fmt, ...)
    __attribute__ ((format (printf, 4, 5)));

#endif

/* vim: set sw=4 sts=4 fdm=syntax et : */

