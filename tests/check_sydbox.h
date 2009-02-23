/* Sydbox unit tests
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@googlemail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef SYDBOX_GUARD_TEST_SYDBOX_H
#define SYDBOX_GUARD_TEST_SYDBOX_H 1

Suite *util_suite_create(void);
Suite *path_suite_create(void);
Suite *children_suite_create(void);
Suite *trace_suite_create(void);
Suite *syscall_suite_create(void);

#endif /* SYDBOX_GUARD_TEST_SYDBOX_H */
