#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

clean_files+=( "arnold.layne.hard" )

start_test "t06-link-deny"
sydbox -- ./t06_link
if [[ 0 == $? ]]; then
    die "failed to deny link"
fi
end_test

start_test "t06-link-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t06_link
if [[ 0 != $? ]]; then
    die "failed to predict link"
fi
end_test

start_test "t06-link-write"
SANDBOX_WRITE="${cwd}" sydbox -- ./t06_link
if [[ 0 != $? ]]; then
    die "write didn't allow access"
elif [[ ! -f arnold.layne.hard ]]; then
    die "file doesn't exist, write didn't allow access"
fi
end_test
