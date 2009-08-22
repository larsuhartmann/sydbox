/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
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

#ifndef SYDBOX_GUARD_TRACE_H
#define SYDBOX_GUARD_TRACE_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <errno.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif // HAVE_SYS_REG_H

/* We need additional hackery on IA64 to include linux/ptrace.h.
 */
#if defined(IA64)
#ifdef HAVE_STRUCT_IA64_FPREG
#define ia64_fpreg XXX_ia64_fpreg
#endif // HAVE_STRUCT_IA64_FPREG
#ifdef HAVE_STRUCT_PT_ALL_USER_REGS
#define pt_all_user_regs XXX_pt_all_user_regs
#endif // HAVE_STRUCT_PT_ALL_USER_REGS
#endif // defined(IA64)
#include <linux/ptrace.h>
#if defined(IA64)
#undef ia64_fpreg
#undef pt_all_user_regs
#endif // defined(IA64)

#include "sydbox-log.h"

#define ADDR_MUL        ((64 == __WORDSIZE) ? 8 : 4)
#define MAX_ARGS        6

/**
 * Events
 */
enum
{
    E_STOP = 0,     /**< Child was stopped */
    E_SYSCALL,      /**< Child is entering or exiting a system call. */
    E_FORK,         /**< Child is entering a fork() call. */
    E_VFORK,        /**< Child is entering a vfork() call.*/
    E_CLONE,        /**< Child is entering a clone call. */
    E_EXEC,         /**< Child is entering an execve() call. */
    E_GENUINE,      /**< Child has receieved a genuine signal. */
    E_EXIT,         /**< Child has exited. */
    E_EXIT_SIGNAL,  /**< Child has exited with a signal. */
    E_UNKNOWN       /**< Child has received an unknown signal. */
};

/**
 * Returns the personality of the process with the given PID.
 */
int trace_personality(pid_t pid);

/**
 * Given status information returned by waitpid(), returns the event.
 */
unsigned int trace_event(int status);

/**
 * Indicates that this process is to be traced by its parent.
 * Returns 0 on success, -1 on failure and sets errno accordingly.
 */
int trace_me(void);

/**
 * Sets up ptrace() options.
 * Returns 0 on success, -1 on failure and sets errno accordingly.
 */
int trace_setup(pid_t pid);

/**
 * Lets the given child continue its execution untraced.
 * Returns 0 on success, -1 on failure and sets errno accordingly.
 */
int trace_cont(pid_t pid);

/**
 * Kills the given child.
 * Returns 0 on success or if child is already dead, -1 on failure and sets
 * errno accordingly.
 */
int trace_kill(pid_t pid);

/**
 * Restarts the child and arranges it to stop at the next system call.
 * Returns 0 on success, -1 on failure and sets errno accordingly.
 */
int trace_syscall(pid_t pid, int data);

/**
 * Retrieve a message (as an unsigned long) about the ptrace event that just
 * happened.
 * Returns 0 on success, -1 on failure and sets errno accordingly.
 */
int trace_geteventmsg(pid_t pid, void *data);

/**
 * Get the system call number and place it in scno.
 * Returns 0 on success, -1 on failure and sets errno accordingly.
 */
int trace_get_syscall(pid_t pid, long *scno);

/**
 * Set the system call to the given value.
 * Returns 0 on success, -1 on failure and sets errno accordingly.
 */
int trace_set_syscall(pid_t pid, long scno);

/**
 * Get the system call return value and place it in res.
 * Returns 0 on success, -1 on failure and sets errno accordingly.
 */
int trace_get_return(pid_t pid, long *res);

/**
 * Set the system call return value to the given value.
 * Returns 0 on success, -1 on failure and sets errno accordingly.
 */
int trace_set_return(pid_t pid, long val);

/**
 * Get the given argument and place it in res.
 * Returns 0 on success, -1 on failure and sets errno accordingly.
 */
int trace_get_arg(pid_t pid, int personality, int arg, long *res);

/**
 * Get the requested path argument.
 * Returns the string on success, NULL on failure and sets errno accordingly.
 */
char *trace_get_path(pid_t pid, int personality, int arg);

/**
 * Sets the path argument to the given value.
 * Returns 0 on success, -1 on failure and sets errno accordingly.
 */
int trace_set_path(pid_t pid, int personality, int arg, const char *src, size_t len);

/**
 * Fake the stat buffer.
 * Returns 0 on success, -1 on failure and sets errno accordingly.
 */
int trace_fake_stat(pid_t pid, int personality);

#endif // SYDBOX_GUARD_TRACE_H

