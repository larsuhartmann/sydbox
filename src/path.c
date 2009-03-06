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

int pathnode_new(struct pathnode **head, const char *path) {
    char path_simple[PATH_MAX];
    struct pathnode *newnode;

    if (NULL == path) {
        LOGD("Path is NULL not adding to list");
        return -1;
    }
    newnode = (struct pathnode *) xmalloc(sizeof(struct pathnode));
    remove_slash(path_simple, path);
    newnode->path = xmalloc(PATH_MAX * sizeof(char));
    shell_expand(newnode->path, path_simple);
    newnode->next = *head; // link next
    *head = newnode; // link head
    LOGV("New path item \"%s\"", newnode->path);
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
    struct pathnode *temp;
    struct pathnode *previous, *current;

    if (0 == strncmp(path, (*head)->path, PATH_MAX)) { // Deleting first node
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
        while (NULL != current && 0 == strncmp(path, current->path, PATH_MAX)) {
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
    char item[PATH_MAX];
    int pos, itemlen, numpaths = 0;
    char *delim;

    if (NULL == pathlist_env) {
        LOGV("The given environment variable isn't set");
        return 0;
    }

    // Use a loop with strchr, because strtok sucks
    pos = 0;
    while (pos < strlen(pathlist_env)) {
        delim = strchr(pathlist_env + pos, ':');
        itemlen = delim ? delim - (pathlist_env + pos) : strlen(pathlist_env) - pos;
        if (0 == itemlen)
            LOGW("Ignoring empty path element in position %d", numpaths);
        else {
            memcpy(item, pathlist_env + pos, itemlen);
            item[itemlen] = '\0';
            pathnode_new(pathlist, item);
            ++numpaths;
        }
        pos += ++itemlen;
    }
    LOGV("Initialized path list with %d paths", numpaths);
    return numpaths;
}

int pathlist_check(struct pathnode **pathlist, const char *path) {
    int ret;
    char path_simple[PATH_MAX];
    struct pathnode *node;

    LOGD("Checking \"%s\"", path);
    remove_slash(path_simple, path);

    ret = 0;
    node = *pathlist;
    while (NULL != node) {
        if (0 == strncmp(path_simple, node->path, strlen(node->path))) {
            if (strlen(path_simple) > strlen(node->path)) {
                /* Path begins with one of the allowed paths. Check for a
                 * zero byte or a / on the next character so that for example
                 * /devzero/foo doesn't pass the test when /dev is the only
                 * allowed path.
                 */
                const char last = path_simple[strlen(node->path)];
                if ('\0' == last || '/' == last) {
                    LOGD("\"%s\" begins with \"%s\"", path_simple, node->path);
                    ret = 1;
                    break;
                }
                else
                    LOGD("\"%s\" doesn't begin with \"%s\"", path_simple, node->path);
            }
            else {
                LOGD("\"%s\" begins with \"%s\"", path_simple, node->path);
                ret = 1;
                break;
            }
        }
        else
            LOGD("\"%s\" doesn't begin with \"%s\"", path_simple, node->path);
        node = node->next;
    }
    if (ret)
        LOGD("Path list check succeeded for \"%s\"", path_simple);
    else
        LOGD("Path list check failed for \"%s\"", path_simple);
    return ret;
}
