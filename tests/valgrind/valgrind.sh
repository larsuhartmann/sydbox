#!/bin/sh
# vim: set sw=4 et sts=4 tw=80 :

G_SLICE=always-malloc \
    valgrind -q --error-exitcode=126 \
    --leak-check=full \
    --suppressions="$SYDBOX_VALGRIND"/default.supp \
    --track-origins=yes \
    --log-fd=4 \
    --input-fd=4 \
    "$@"

