#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

rm -fr arnold.layne
if [[ 0 != $? ]]; then
    die "rm -fr arnold.layne"
fi

say "t06-link-deny"
sydbox -- ./t06_link
if [[ 0 == $? ]]; then
    die "failed to deny link"
fi

say "t06-link-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t06_link
if [[ 0 != $? ]]; then
    die "failed to predict link"
fi

say "t06-link-write"
SANDBOX_WRITE="${cwd}" sydbox -- ./t06_link
if [[ 0 != $? ]]; then
    die "write didn't allow access"
elif [[ ! -f arnold.layne ]]; then
    die "file doesn't exist, write didn't allow access"
fi
