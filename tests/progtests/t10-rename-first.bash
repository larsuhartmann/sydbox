#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

clean_files+=( "lucifer.sam" )

start_test "t10-rename-first-deny"
sydbox -- ./t10_rename_first
if [[ 0 == $? ]]; then
    die "failed to deny rename"
elif [[ -f lucifer.sam ]]; then
    die "file exists, failed to deny rename"
fi
end_test

start_test "t10-rename-first-write"
SYDBOX_WRITE="${cwd}" sydbox -- ./t10_rename_first
if [[ 0 != $? ]]; then
    die "failed to allow rename"
elif [[ ! -f lucifer.sam ]]; then
    die "file doesn't exist, failed to allow rename"
fi
end_test
