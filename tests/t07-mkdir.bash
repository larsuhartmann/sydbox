#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

rm -f arnold.layne
if [[ 0 != $? ]]; then
    die "rm -f arnold.layne"
fi

say "t07-mkdir-deny"
sydbox -- ./t07_mkdir
if [[ 0 == $? ]]; then
    die "failed to deny mkdir"
elif [[ -d arnold.layne ]]; then
    die "dir exists, failed to deny mkdir"
fi

say "t07-mkdir-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t07_mkdir
if [[ 0 != $? ]]; then
    die "failed to predict mkdir"
elif [[ -d arnold.layne ]]; then
    die "predict allowed access"
fi

say "t07-mkdir-write"
SANDBOX_WRITE="${cwd}" sydbox -- ./t07_mkdir
if [[ 0 != $? ]]; then
    die "write didn't allow access"
elif [[ ! -d arnold.layne ]]; then
    die "dir doesn't exist, write didn't allow access"
fi
