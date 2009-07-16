/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Saleem Abdulrasool <compnerd@compnerd.org>
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

#ifndef __TRACE_H__
#define __TRACE_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

#if defined(X86_64)
int trace_type(pid_t pid);
#endif // defined(X86_64)

int
trace_syscall (pid_t pid, int data);

int
trace_geteventmsg (pid_t pid, void *data);

int
trace_get_arg (pid_t pid, int personality, int arg, long *res);

#if 0
int
trace_set_arg (pid_t pid, int arg, long val);
#endif

int
trace_get_syscall (pid_t pid, long *syscall);

int
trace_set_syscall (pid_t pid, long syscall);

int
trace_get_return (pid_t pid, long *res);

int
trace_set_return (pid_t pid, long val);

char *
trace_get_string (pid_t pid, int personality, int arg);

int
trace_set_string (pid_t pid, int personality, int arg, const char *src, size_t len);

int
trace_fake_stat (pid_t pid, int personality);

#endif

