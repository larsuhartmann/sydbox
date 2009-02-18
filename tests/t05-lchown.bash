#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

rm -f arnold.layne
ln -s its.not.the.same arnold.layne
if [[ 0 != $? ]]; then
    die "ln -s its.not.the.same see.emily.play"
fi

say "t05-lchown-deny"
sydbox -- ./t05_lchown
if [[ 0 == $? ]]; then
    die "failed to deny lchown"
fi

say "t05-lchown-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t05_lchown
if [[ 0 != $? ]]; then
    die "failed to predict lchown"
fi

say "t05-lchown-write"
SANDBOX_WRITE="${cwd}" sydbox -- ./t05_lchown
if [[ 0 != $? ]]; then
    die "write didn't allow access"
fi
