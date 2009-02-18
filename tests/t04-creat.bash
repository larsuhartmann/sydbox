#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

rm arnold.layne

say "t04-creat-deny"
sydbox -- ./t04_creat
if [[ 0 == $? ]]; then
    die "failed to deny creat"
elif [[ -f arnold.layne ]]; then
    die "file exists, failed to deny creat"
fi

say "t04-creat-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t04_creat
if [[ 0 != $? ]]; then
    die "failed to predict creat"
elif [[ -f arnold.layne ]]; then
    die "predict allowed access"
fi

say "t04-creat-write"
SANDBOX_WRITE="${cwd}" sydbox -- ./t04_creat
if [[ 0 != $? ]]; then
    die "failed to allow creat"
elif [[ ! -f arnold.layne ]]; then
    die "file doesn't exist, failed to allow creat"
fi
