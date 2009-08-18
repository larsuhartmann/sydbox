#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

clean_files+=( "arnold.layne.hard" )

start_test "t25-linkat-first-deny"
sydbox -- ./t25_linkat_first
if [[ 0 == $? ]]; then
    die "failed to deny linkat"
elif [[ -f arnold.layne.hard ]]; then
    die "file exists, failed to deny linkat"
fi
end_test

start_test "t25-linkat-first-predict"
SYDBOX_PREDICT="${cwd}" sydbox -- ./t25_linkat_first
if [[ 0 != $? ]]; then
    die "failed to predict linkat"
elif [[ -f arnold.layne.hard ]]; then
    die "predict allowed access"
fi
end_test

start_test "t25-linkat-first-write"
SYDBOX_WRITE="${cwd}" sydbox -- ./t25_linkat_first
if [[ 0 != $? ]]; then
    die "failed to allow linkat"
elif [[ ! -f arnold.layne.hard ]]; then
    die "file doesn't exist, failed to allow linkat"
fi
end_test
