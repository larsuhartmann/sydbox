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

#include <stdbool.h>
#include <string.h>

#include <glib.h>

#include "net.h"

bool net_localhost(const char *addr)
{
    return (0 == strncmp(addr, "127.0.0.1", 10) || 0 == strncmp(addr, "::1", 4));
}

void netlist_new(GSList **netlist, int family, int port, const char *addr)
{
    struct sydbox_addr *saddr = (struct sydbox_addr *) g_malloc0(sizeof(struct sydbox_addr));

    saddr->family = family;
    saddr->port = port;
    if (NULL != addr)
        saddr->addr = g_strdup(addr);

    *netlist = g_slist_prepend(*netlist, saddr);
}

static void netlist_free_one(struct sydbox_addr *saddr, void *userdata G_GNUC_UNUSED)
{
    g_free(saddr->addr);
    g_free(saddr);
}

void netlist_free(GSList **netlist)
{
    g_slist_foreach(*netlist, (GFunc) netlist_free_one, NULL);
    g_slist_free(*netlist);
    *netlist = NULL;
}

