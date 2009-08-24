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

#ifndef SYDBOX_GUARD_FLAGS_H
#define SYDBOX_GUARD_FLAGS_H 1

// System call dispatch flags
#define RETURNS_FD              (1 << 0)  // The function returns a file descriptor
#define OPEN_MODE               (1 << 1)  // Check the mode argument of open()
#define OPEN_MODE_AT            (1 << 2)  // Check the mode argument of openat()
#define ACCESS_MODE             (1 << 3)  // Check the mode argument of access()
#define ACCESS_MODE_AT          (1 << 4)  // Check the mode argument of faccessat()
#define CHECK_PATH              (1 << 5)  // First argument should be a valid path
#define CHECK_PATH2             (1 << 6)  // Second argument should be a valid path
#define CHECK_PATH_AT           (1 << 7)  // CHECK_PATH for at suffixed functions
#define CHECK_PATH_AT1          (1 << 8)  // CHECK_PATH2 for symlinkat()
#define CHECK_PATH_AT2          (1 << 9)  // CHECK_PATH2 for at suffixed functions
#define DONT_RESOLV             (1 << 10) // Don't resolve symlinks
#define IF_AT_SYMLINK_FOLLOW4   (1 << 11) // Resolving path depends on AT_SYMLINK_FOLLOW (4th argument)
#define IF_AT_SYMLINK_NOFOLLOW3 (1 << 12) // Resolving path depends on AT_SYMLINK_NOFOLLOW (3th argument)
#define IF_AT_SYMLINK_NOFOLLOW4 (1 << 13) // Resolving path depends on AT_SYMLINK_NOFOLLOW (4th argument)
#define IF_AT_REMOVEDIR2        (1 << 14) // Resolving path depends on AT_REMOVEDIR (2nd argument)
#define CAN_CREAT               (1 << 15) // The system call can create the first path if it doesn't exist
#define CAN_CREAT2              (1 << 16) // The system call can create the second path if it doesn't exist
#define CAN_CREAT_AT            (1 << 17) // CAN_CREAT for at suffixed functions
#define CAN_CREAT_AT2           (1 << 18) // CAN_CREAT2 for at suffixed functions
#define MUST_CREAT              (1 << 19) // The system call _must_ create the first path, fails otherwise
#define MUST_CREAT2             (1 << 20) // The system call _must_ create the second path, fails otherwise
#define MUST_CREAT_AT           (1 << 21) // MUST_CREAT for at suffixed functions
#define MUST_CREAT_AT1          (1 << 22) // MUST_CREAT2 for symlinkat()
#define MUST_CREAT_AT2          (1 << 23) // MUST_CREAT2 for at suffixed functions
#define MAGIC_STAT              (1 << 24) // Check if the stat() call is magic
#define CONNECT_CALL            (1 << 25) // Check if the connect() call matches the accepted connect IPs
#define BIND_CALL               (1 << 26) // Check if the bind() call matches the accepted bind IPs
#define NET_CALL                (1 << 27) // Accepting the system call depends on the net flag
#define EXEC_CALL               (1 << 28) // Allowing the system call depends on the exec flag

#endif // SYDBOX_GUARD_FLAGS_H

