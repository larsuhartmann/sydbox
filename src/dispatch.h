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

#ifndef SYDBOX_GUARD_DISPATCH_H
#define SYDBOX_GUARD_DISPATCH_H 1

#include <stdbool.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#define IS_CHDIR(_sno)      (__NR_chdir == (_sno) || __NR_fchdir == (_sno))
#define UNKNOWN_SYSCALL     "unknown"

#if defined(I386) || defined(IA64)
int dispatch_flags(int personality, int sno);
const char *dispatch_name(int personality, int sno);
const char *dispatch_mode(int personality);
bool dispatch_chdir(int personality, int sno);
#elif defined(X86_64)
int dispatch_flags32(int sno);
int dispatch_flags64(int sno);
const char *dispatch_name32(int sno);
const char *dispatch_name64(int sno);
bool dispatch_chdir32(int sno);
bool dispatch_chdir64(int sno);

#define dispatch_flags(personality, sno) \
    ((personality) == 0) ? dispatch_flags32((sno)) : dispatch_flags64((sno))
#define dispatch_name(personality, sno) \
    ((personality) == 0) ? dispatch_name32((sno)) : dispatch_name64((sno))
#define dispatch_mode(personality) \
    ((personality) == 0) ? "32 bit" : "64 bit"
#define dispatch_chdir(personality, sno) \
    ((personality) == 0) ? dispatch_chdir32((sno)) : dispatch_chdir64((sno))

#else
#error unsupported architecture
#endif

#endif // SYDBOX_GUARD_DISPATCH_H

