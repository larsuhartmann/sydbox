#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

start_test "t16-umount-deny"
sydbox -- ./t16_umount
if [[ 0 == $? ]]; then
    die "failed to deny umount"
fi
end_test

start_test "t16-umount-predict"
SYDBOX_PREDICT="${cwd}" sydbox -- ./t16_umount
if [[ 0 != $? ]]; then
    die "failed to predict umount"
fi
end_test
