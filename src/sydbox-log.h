/* vim: set sw=4 sts=4 fdm=syntax et : */

/*
 * Copyright 2009 Saleem Abdulrasool <compnerd@compnerd.org>
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

#ifndef __SYDBOX_LOG_H__
#define __SYDBOX_LOG_H__

#include <glib.h>

/**
 * LOG_LEVEL_DEBUG_TRACE:
 *
 * Custom log level used for extremely verbose logging
 *
 * Since: 0.1_alpha
 **/
#define LOG_LEVEL_DEBUG_TRACE       (1 << (G_LOG_LEVEL_USER_SHIFT + 0))

/**
 * g_info:
 * @varargs: format string, followed by parameters to insert into the format
 * string (as with printf())
 *
 * A convinience function/macro to log an info message
 *
 * Since: 0.1_alpha
 **/
#ifndef g_info
#define g_info(...)                 g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, __VA_ARGS__)
#endif

#ifndef g_debug_trace
#define g_debug_trace(...)          g_log (G_LOG_DOMAIN, LOG_LEVEL_DEBUG_TRACE, __VA_ARGS__)
#endif

/**
 * sydbox_log_init:
 *
 * Initalises the logging infrastructure
 *
 * Since: 0.1_alpha
 **/
void
sydbox_log_init (void);

/**
 * sydbox_log_fini:
 *
 * Shutdown the logging infrastructure and perform any cleanup necessary
 *
 * Since: 0.1_alpha
 **/
void
sydbox_log_fini (void);

#endif

