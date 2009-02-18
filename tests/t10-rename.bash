#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

# FIXME this testcase doesn't check if the second path is checked correctly.

. test-lib.bash

trap 'rm -f its.not.the.same' EXIT

say "t10-rename-deny"
sydbox -- ./t10_rename
if [[ 0 == $? ]]; then
    die "failed to deny rename"
elif [[ -f its.not.the.same ]]; then
    die "file exists, failed to deny rename"
fi

say "t10-rename-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t10_rename
if [[ 0 != $? ]]; then
    die "failed to predict rename"
elif [[ -f its.not.the.same ]]; then
    die "predict allowed access"
fi

say "t10-rename-write"
SANDBOX_WRITE="${cwd}" sydbox -- ./t10_rename
if [[ 0 != $? ]]; then
    die "failed to allow rename"
elif [[ ! -f its.not.the.same ]]; then
    die "file doesn't exist, failed to allow rename"
fi
