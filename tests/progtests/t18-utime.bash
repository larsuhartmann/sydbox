#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

start_test "t18-utime-deny"
sydbox -- ./t18_utime
if [[ 0 == $? ]]; then
    echo "Failed to deny utime"
fi
end_test

start_test "t18-utime-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t18_utime
if [[ 0 != $? ]]; then
    echo "Failed to predict utime"
fi
end_test

# No t18-utime-write because of possible noatime, nomtime mount options
