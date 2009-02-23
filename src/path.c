/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel
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

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "defs.h"

int path_magic_dir(const char *pathname) {
    char mdir[PATH_MAX];

    strncpy(mdir, CMD_PATH, CMD_PATH_LEN + 1);
    // Remove the trailing slash
    mdir[CMD_PATH_LEN - 1] = '\0';
    if (0 == strncmp(pathname, mdir, CMD_PATH_LEN))
        return 1;

    strncpy(mdir, CMD_WRITE, CMD_WRITE_LEN + 1);
    // Remove the trailing slash
    mdir[CMD_WRITE_LEN - 1] = '\0';
    if (0 == strncmp(pathname, mdir, CMD_WRITE_LEN))
        return 1;

    strncpy(mdir, CMD_PREDICT, CMD_PREDICT_LEN + 1);
    // Remove the trailing slash
    mdir[CMD_PREDICT_LEN - 1] = '\0';
    if (0 == strncmp(pathname, mdir, CMD_PREDICT_LEN))
        return 1;

    return 0;
}

int path_magic_write(const char *pathname) {
    if (0 == strncmp(pathname, CMD_WRITE, CMD_WRITE_LEN))
        return 1;
    else
        return 0;
}

int path_magic_predict(const char *pathname) {
    if (0 == strncmp(pathname, CMD_PREDICT, CMD_PREDICT_LEN))
        return 1;
    else
        return 0;
}

void pathnode_new(struct pathnode **head, const char *pathname) {
    char path_simple[PATH_MAX];
    struct pathnode *newnode;

    newnode = (struct pathnode *) xmalloc(sizeof(struct pathnode));
    remove_slash(pathname, path_simple);
    newnode->pathname = xmalloc(PATH_MAX * sizeof(char));
    shell_expand(path_simple, newnode->pathname);
    newnode->next = *head; // link next
    *head = newnode; // link head
    lg(LOG_DEBUG, "path.node_new", "New path item \"%s\"", newnode->pathname);
}

void pathnode_free(struct pathnode **head) {
    struct pathnode *current, *temp;

    lg(LOG_DEBUG, "path.node_free", "Freeing pathlist %p", (void *) head);
    current = *head;
    while (NULL != current) {
        temp = current;
        current = current->next;
        free(temp->pathname);
        free(temp);
    }
    *head = NULL;
}

int pathlist_init(struct pathnode **pathlist, const char *pathlist_env) {
    char item[PATH_MAX];
    int pos, itemlen, numpaths = 0;
    char *delim;

    lg(LOG_DEBUG, "path.list_init", "Initializing path list");
    if (NULL == pathlist_env) {
        lg(LOG_DEBUG, "path.list_init.env_unset", "The given environment variable isn't set");
        return 0;
    }

    // Use a loop with strchr, because strtok sucks
    pos = 0;
    while (pos < strlen(pathlist_env)) {
        delim = strchr(pathlist_env + pos, ':');
        itemlen = delim ? delim - (pathlist_env + pos) : strlen(pathlist_env) - pos;
        if (0 == itemlen)
            lg(LOG_WARNING, "path.list_init",
                    "Ignoring empty path element in position %d", numpaths);
        else {
            memcpy(item, pathlist_env + pos, itemlen);
            item[itemlen] = '\0';
            pathnode_new(pathlist, item);
            ++numpaths;
        }
        pos += ++itemlen;
    }
    lg(LOG_DEBUG, "path.list_init.done",
            "Initialized path list with %d pathnames", numpaths);
    return numpaths;
}

int pathlist_check(struct pathnode **pathlist, const char *pathname) {
    int ret;
    char path_simple[PATH_MAX];
    struct pathnode *node;

    lg(LOG_DEBUG, "path.list_check", "Checking \"%s\"", pathname);
    remove_slash(pathname, path_simple);

    ret = 0;
    node = *pathlist;
    while (NULL != node) {
        if (0 == strncmp(path_simple, node->pathname, strlen(node->pathname))) {
            if (strlen(path_simple) > strlen(node->pathname)) {
                /* Pathname begins with one of the allowed paths. Check for a
                 * zero byte or a / on the next character so that for example
                 * /devzero/foo doesn't pass the test when /dev is the only
                 * allowed path.
                 */
                const char last = path_simple[strlen(node->pathname)];
                if ('\0' == last || '/' == last) {
                    lg(LOG_DEBUG, "path.list_check.found",
                            "\"%s\" begins with \"%s\"", path_simple,
                            node->pathname);
                    ret = 1;
                    break;
                }
                else
                    lg(LOG_DEBUG, "path.list_check.not_found",
                            "\"%s\" doesn't begin with \"%s\"",
                            path_simple, node->pathname);
            }
            else {
                lg(LOG_DEBUG, "path.list_check.found",
                        "\"%s\" begins with \"%s\"", path_simple, node->pathname);
                ret = 1;
                break;
            }
        }
        else
            lg(LOG_DEBUG, "path.list_check.not_found",
                    "\"%s\" doesn't begin with \"%s\"", path_simple, node->pathname);
        node = node->next;
    }
    if (ret)
        lg(LOG_DEBUG, "path.list_check.success",
                "Path list check succeeded for \"%s\"", path_simple);
    else
        lg(LOG_DEBUG, "path.list_check.fail",
                "Path list check failed for \"%s\"", path_simple);
    return ret;
}
