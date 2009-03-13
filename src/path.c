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

int path_magic_dir(const char *path) {
    if (0 == strncmp(path, CMD_PATH, CMD_PATH_LEN - 1))
        return 1;
    else
        return 0;
}

int path_magic_on(const char *path) {
    if (0 == strncmp(path, CMD_ON, CMD_ON_LEN))
        return 1;
    else
        return 0;
}

int path_magic_off(const char *path) {
    if (0 == strncmp(path, CMD_OFF, CMD_OFF_LEN))
        return 1;
    else
        return 0;
}

int path_magic_toggle(const char *path) {
    if (0 == strncmp(path, CMD_TOGGLE, CMD_TOGGLE_LEN))
        return 1;
    else
        return 0;
}

int path_magic_lock(const char *path) {
    if (0 == strncmp(path, CMD_LOCK, CMD_LOCK_LEN))
        return 1;
    else
        return 0;
}

int path_magic_exec_lock(const char *path) {
    if (0 == strncmp(path, CMD_EXEC_LOCK, CMD_EXEC_LOCK_LEN))
        return 1;
    else
        return 0;
}

int path_magic_write(const char *path) {
    if (0 == strncmp(path, CMD_WRITE, CMD_WRITE_LEN))
        return 1;
    else
        return 0;
}

int path_magic_predict(const char *path) {
    if (0 == strncmp(path, CMD_PREDICT, CMD_PREDICT_LEN))
        return 1;
    else
        return 0;
}

int path_magic_rmwrite(const char *path) {
    if (0 == strncmp(path, CMD_RMWRITE, CMD_RMWRITE_LEN))
        return 1;
    else
        return 0;
}

int path_magic_rmpredict(const char *path) {
    if (0 == strncmp(path, CMD_RMPREDICT, CMD_RMPREDICT_LEN))
        return 1;
    else
        return 0;
}

int pathnode_new(struct pathnode **head, const char *path, int sanitize) {
    struct pathnode *newnode;

    if (NULL == path) {
        LOGV("Path is NULL not adding to list");
        return -1;
    }
    else if (0 == strlen(path)) {
        LOGV("Path is empty not adding to list");
        return -1;
    }

    newnode = (struct pathnode *) xmalloc(sizeof(struct pathnode));
    if (!sanitize)
        newnode->path = xstrndup(path, strlen(path) + 1);
    else {
        char *spath = remove_slash(path);
        newnode->path = shell_expand(spath);
        free(spath);
        LOGV("New path item \"%s\"", newnode->path);
    }
    newnode->next = *head; // link next
    *head = newnode; // link head
    return 0;
}

void pathnode_free(struct pathnode **head) {
    struct pathnode *current, *temp;

    LOGD("Freeing pathlist %p", (void *) head);
    current = *head;
    while (NULL != current) {
        temp = current;
        current = current->next;
        free(temp->path);
        free(temp);
    }
    *head = NULL;
}

void pathnode_delete(struct pathnode **head, const char *path) {
    int len = strlen(path) + 1;
    struct pathnode *temp;
    struct pathnode *previous, *current;

    if (0 == strncmp(path, (*head)->path, len)) { // Deleting first node
        temp = *head;
        *head = (*head)->next;
        if (NULL != temp->path)
            free(temp->path);
        free(temp);
    }
    else {
        previous = *head;
        current = (*head)->next;

        // Find the correct location
        while (NULL != current && 0 == strncmp(path, current->path, len)) {
            previous = current;
            current = current->next;
        }

        if (NULL != current) {
            temp = current;
            previous->next = current->next;
            if (NULL != temp->path)
                free(temp->path);
            free(temp);
        }
    }
}

int pathlist_init(struct pathnode **pathlist, const char *pathlist_env) {
    char *item;
    unsigned int itemlen, numpaths = 0, pos = 0;
    char *delim;

    if (NULL == pathlist_env) {
        LOGV("The given environment variable isn't set");
        return 0;
    }

    // Use a loop with strchr, because strtok sucks
    while (pos < strlen(pathlist_env)) {
        delim = strchr(pathlist_env + pos, ':');
        itemlen = delim ? delim - (pathlist_env + pos) : (unsigned int) (strlen(pathlist_env) - pos);
        if (0 == itemlen)
            LOGW("Ignoring empty path element in position %d", numpaths);
        else {
            item = xmalloc(itemlen * sizeof(char));
            memcpy(item, pathlist_env + pos, itemlen);
            item[itemlen] = '\0';
            pathnode_new(pathlist, item, 1);
            free(item);
            ++numpaths;
        }
        pos += ++itemlen;
    }
    LOGV("Initialized path list with %d paths", numpaths);
    return numpaths;
}

int pathlist_check(struct pathnode **pathlist, const char *path_sanitized) {
    int ret;
    struct pathnode *node;

    LOGD("Checking `%s'", path_sanitized);

    ret = 0;
    node = *pathlist;
    while (NULL != node) {
        if (0 == strncmp(path_sanitized, node->path, strlen(node->path))) {
            if (strlen(path_sanitized) > strlen(node->path)) {
                /* Path begins with one of the allowed paths. Check for a
                 * zero byte or a / on the next character so that for example
                 * /devzero/foo doesn't pass the test when /dev is the only
                 * allowed path.
                 */
                const char last = path_sanitized[strlen(node->path)];
                if ('\0' == last || '/' == last) {
                    LOGD("`%s' begins with `%s'", path_sanitized, node->path);
                    ret = 1;
                    break;
                }
                else
                    LOGD("`%s' doesn't begin with `%s'", path_sanitized, node->path);
            }
            else {
                LOGD("`%s' begins with `%s'", path_sanitized, node->path);
                ret = 1;
                break;
            }
        }
        else
            LOGD("`%s' doesn't begin with `%s'", path_sanitized, node->path);
        node = node->next;
    }
    if (ret)
        LOGD("Path list check succeeded for `%s'", path_sanitized);
    else
        LOGD("Path list check failed for `%s'", path_sanitized);
    return ret;
}
