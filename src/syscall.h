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

#ifndef SYDBOX_GUARD_SYSCALL_H
#define SYDBOX_GUARD_SYSCALL_H 1

#include <sysexits.h>

#include <glib-object.h>

#include "children.h"
#include "context.h"

enum {
    RS_ALLOW,
    RS_NOWRITE,
    RS_MAGIC,
    RS_DENY,
    RS_ERROR = EX_SOFTWARE
};

struct checkdata {
    gint result;            // Check result
    gint save_errno;        // errno when the result is RS_ERROR

    gboolean resolve;       // TRUE if the system call resolves paths
    glong open_flags;       // flags argument of open()/openat()
    glong access_flags;     // flags argument of access()/faccessat()
    gchar *dirfdlist[2];    // dirfd arguments (resolved)
    gchar *pathlist[4];     // Path arguments
    gchar *rpathlist[4];    // Path arguments (canonicalized)
};

typedef struct _SystemCall {
    GObject parent_instance;

    guint no;
    guint flags;
} SystemCall;

typedef struct _SystemCallClass {
    GObjectClass parent_class;

    void (*start_check)(SystemCall *, gpointer, gpointer, gpointer);
    void (*flags)(SystemCall *, gpointer, gpointer, gpointer);
    void (*magic)(SystemCall *, gpointer, gpointer, gpointer);
    void (*resolve)(SystemCall *, gpointer, gpointer, gpointer);
    void (*canonicalize)(SystemCall *, gpointer, gpointer, gpointer);
    void (*make_absolute)(SystemCall *, gpointer, gpointer, gpointer);
    void (*check)(SystemCall *, gpointer, gpointer, gpointer);
    void (*end_check)(SystemCall *, gpointer, gpointer, gpointer);
} SystemCallClass;

#define TYPE_SYSTEMCALL             (systemcall_get_type())
#define SYSTEMCALL(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_SYSTEMCALL, SystemCall))
#define SYSTEMCALL_CLASS(cls)       (G_TYPE_CHECK_CLASS_CAST((cls), TYPE_SYSTEMCALL, SystemCallClass))
#define IS_SYSTEMCALL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_SYSTEMCALL))
#define IS_SYSTEMCALL_CLASS(cls)    (G_TYPE_CHECK_CLASS_TYPE((cls), TYPE_SYSTEMCALL))
#define SYSTEMCALL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), TYPE_SYSTEMCALL, SystemCallClass))

GType systemcall_get_type(void);

void syscall_init(void);
void syscall_free(void);
SystemCall *syscall_get_handler(int no);
int syscall_handle(context_t *ctx, struct tchild *child);

#endif // SYDBOX_GUARD_SYSCALL_H

