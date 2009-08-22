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

#ifndef SYDBOX_GUARD_COMMANDS_H
#define SYDBOX_GUARD_COMMANDS_H 1

#define SYDBOX_CMD_PATH                "/dev/sydbox/"
#define SYDBOX_CMD_PATH_LEN            12
#define SYDBOX_CMD_ON                  SYDBOX_CMD_PATH"on"
#define SYDBOX_CMD_ON_LEN              (SYDBOX_CMD_PATH_LEN + 3)
#define SYDBOX_CMD_OFF                 SYDBOX_CMD_PATH"off"
#define SYDBOX_CMD_OFF_LEN             (SYDBOX_CMD_PATH_LEN + 4)
#define SYDBOX_CMD_TOGGLE              SYDBOX_CMD_PATH"toggle"
#define SYDBOX_CMD_TOGGLE_LEN          (SYDBOX_CMD_PATH_LEN + 7)
#define SYDBOX_CMD_ENABLED             SYDBOX_CMD_PATH"enabled"
#define SYDBOX_CMD_ENABLED_LEN         (SYDBOX_CMD_PATH_LEN + 8)
#define SYDBOX_CMD_LOCK                SYDBOX_CMD_PATH"lock"
#define SYDBOX_CMD_LOCK_LEN            (SYDBOX_CMD_PATH_LEN + 5)
#define SYDBOX_CMD_EXEC_LOCK           SYDBOX_CMD_PATH"exec_lock"
#define SYDBOX_CMD_EXEC_LOCK_LEN       (SYDBOX_CMD_PATH_LEN + 10)
#define SYDBOX_CMD_WRITE               SYDBOX_CMD_PATH"write/"
#define SYDBOX_CMD_WRITE_LEN           (SYDBOX_CMD_PATH_LEN + 6)
#define SYDBOX_CMD_RMWRITE             SYDBOX_CMD_PATH"unwrite/"
#define SYDBOX_CMD_RMWRITE_LEN         (SYDBOX_CMD_PATH_LEN + 8)
#define SYDBOX_CMD_SANDBOX_EXEC        SYDBOX_CMD_PATH"sandbox_exec"
#define SYDBOX_CMD_SANDBOX_EXEC_LEN    (SYDBOX_CMD_PATH_LEN + 13)
#define SYDBOX_CMD_UNSANDBOX_EXEC      SYDBOX_CMD_PATH"unsandbox_exec"
#define SYDBOX_CMD_UNSANDBOX_EXEC_LEN  (SYDBOX_CMD_PATH_LEN + 15)
#define SYDBOX_CMD_ADDFILTER           SYDBOX_CMD_PATH"addfilter/"
#define SYDBOX_CMD_ADDFILTER_LEN       (SYDBOX_CMD_PATH_LEN + 10)
#define SYDBOX_CMD_RMFILTER            SYDBOX_CMD_PATH"rmfilter/"
#define SYDBOX_CMD_RMFILTER_LEN        (SYDBOX_CMD_PATH_LEN + 9)

#endif // SYDBOX_GUARD_COMMANDS_H

