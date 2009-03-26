/**
 * Copyright 2009 Saleem Abdulrasool <compnerd@compnerd.org>
 **/

#ifndef __WRAPPERS_H__
#define __WRAPPERS_H__

typedef enum canonicalize_mode_t
{
    /* All components must exist.  */
    CAN_EXISTING = 0,
    /* All components excluding last one must exist.  */
    CAN_ALL_BUT_LAST = 1,
} canonicalize_mode_t;

char *
edirname (const char *path);

char *
ebasename (const char *path);

char *
ereadlink (const char *path);

char *
canonicalize_filename_mode (const char *name,
                            canonicalize_mode_t can_mode,
                            bool resolve,
                            const char *cwd);

#endif

/* vim: set sw=4 sts=4 fdm=syntax et : */

