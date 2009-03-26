/**
 * Copyright 2009 Saleem Abdulrasool <compnerd@compnerd.org>
 **/

#ifndef __TRACE_H__
#define __TRACE_H__

/* Events */

enum
{
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

unsigned int
trace_event (int status);

int
trace_me (void);

int
trace_setup (pid_t pid);

int
trace_kill (pid_t pid);

int
trace_cont (pid_t pid);

int
trace_syscall (pid_t pid, int data);

int
trace_geteventmsg (pid_t pid, void *data);

int
trace_get_arg (pid_t pid, int arg, long *res);

int
trace_get_syscall (pid_t pid, long *syscall);

int
trace_set_syscall (pid_t pid, long syscall);

int
trace_get_return (pid_t pid, long *res);

int
trace_set_return (pid_t pid, long val);

char *
trace_get_string (pid_t pid, int arg);

int
trace_set_string (pid_t pid, int arg, const char *src, size_t len);

int
trace_fake_stat (pid_t pid);

#endif

/* vim: set sw=4 sts=4 fdm=syntax et : */

