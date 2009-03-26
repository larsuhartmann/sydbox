/* vim: set sw=4 sts=4 fdm=syntax et : */

/**
 * Copyright 2009 Saleem Abdulrasool <compnerd@compnerd.org>
 **/

#ifndef __CHILDREN_H__
#define __CHILDREN_H__

#include <sys/types.h>

#include "path.h"

/* TCHILD flags */
#define TCHILD_NEEDSETUP   (1 << 0)    /* child needs setup */
#define TCHILD_INSYSCALL   (1 << 1)    /* child is in syscall */

#ifndef PID_MAX_LIMIT
#if __WORDSIZE == 64
#define PID_MAX_LIMIT      (1 << 22)
#elif __WORDSIZE == 32
#define PID_MAX_LIMIT      (1 << 15)
#else
#error unsupported wordsize
#endif
#endif

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
   struct pathnode *write_prefixes;
   struct pathnode *predict_prefixes;
};

struct tchild
{
   int flags;                          /* TCHILD_ flags */
   pid_t pid;
   char *cwd;                          /* child's current working directory */
   unsigned long sno;                  /* original syscall no when a system call is faked */
   long retval;                        /* faked syscall will return this value */
   struct tdata *sandbox;
   struct tchild *next;
};

extern struct tchild *childtab[PID_MAX_LIMIT];

void
tchild_new (struct tchild **head, pid_t pid);

void
tchild_free (struct tchild **head);

void
tchild_delete (struct tchild **head, pid_t pid);

#endif

