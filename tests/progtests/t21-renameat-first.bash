#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

clean_files+=( "lucifer.sam" )

start_test "t21-renameat-first-deny"
sydbox -- ./t21_renameat_first
if [[ 0 == $? ]]; then
    die "failed to deny renameat"
elif [[ -f lucifer.sam ]]; then
    die "file exists, failed to deny renameat"
fi
end_test

start_test "t21-renameat-first-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t21_renameat_first
if [[ 0 != $? ]]; then
    die "failed to predict renameat"
elif [[ -f lucifer.sam ]]; then
    die "predict allowed access"
fi
end_test

start_test "t21-renameat-first-write"
SANDBOX_WRITE="${cwd}" sydbox -- ./t21_renameat_first
if [[ 0 != $? ]]; then
    die "failed to allow renameat"
elif [[ ! -f lucifer.sam ]]; then
    die "file doesn't exist, failed to allow renameat"
fi
end_test
