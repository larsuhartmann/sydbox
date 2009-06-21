/* vim: set sw=4 sts=4 fdm=syntax et : */

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

#ifndef __CHILDREN_H__
#define __CHILDREN_H__

#include <sys/types.h>

#include <glib.h>

/* TCHILD flags */
#define TCHILD_NEEDSETUP   (1 << 0)    /* child needs setup */
#define TCHILD_INSYSCALL   (1 << 1)    /* child is in syscall */

/* per process tracking data */
enum lock_status
{
   LOCK_SET,                           /* magic commands are locked */
   LOCK_UNSET,                         /* magic commands are unlocked */
   LOCK_PENDING,                       /* magic commands will be locked when an execve() is encountered */
};

struct tdata
{
   int on;                             /* whether sydbox is on for the child */
   int lock;                           /* whether magic commands are locked for the child */
   int net;                            /* whether child is allowed to access network */
   int exec_banned;                    /* whether execve() calls are banned for child */
   GSList *write_prefixes;
   GSList *predict_prefixes;
};

struct tchild
{
   int flags;                          /* TCHILD_ flags */
   pid_t pid;                          /* pid of child */
   char *cwd;                          /* child's current working directory */
   unsigned long sno;                  /* original syscall no when a system call is faked */
   long retval;                        /* faked syscall will return this value */
   struct tdata *sandbox;              /* sandbox data */
};

void
tchild_new (GSList **children, pid_t pid, pid_t ppid);

void
tchild_free (GSList **children);

void
tchild_delete (GSList **children, pid_t pid);

struct tchild *
tchild_find(GSList *children, pid_t pid);

#endif

