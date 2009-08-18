#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

clean_files+=( "arnold.layne.hard" )

start_test "t26-linkat-second-atfdcwd-deny"
SYDBOX_WRITE="${cwd}/see.emily.play" sydbox -- ./t26_linkat_second_atfdcwd
if [[ 0 == $? ]]; then
    die "failed to deny rename"
elif [[ -f arnold.layne.hard ]]; then
    die "file exists, failed to deny rename"
fi
end_test

start_test "t26-linkat-second-atfdcwd-predict"
SYDBOX_WRITE="${cwd}/see.emily.play" SYDBOX_PREDICT="${cwd}" sydbox -- ./t26_linkat_second_atfdcwd
if [[ 0 != $? ]]; then
    die "failed to predict rename"
elif [[ -f arnold.layne.hard ]]; then
    die "predict allowed access"
fi
end_test

start_test "t26-linkat-second-atfdcwd-write"
SYDBOX_WRITE="${cwd}" sydbox -- ./t26_linkat_second_atfdcwd
if [[ 0 != $? ]]; then
    die "failed to allow linkat"
elif [[ ! -f arnold.layne.hard ]]; then
    die "file doesn't exist, failed to allow linkat"
fi
end_test
