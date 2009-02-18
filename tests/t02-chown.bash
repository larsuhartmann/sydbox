#!/bin/bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

say "t02-chown-deny"
sydbox -- ./t02_chown
if [[ 0 == $? ]]; then
    die "failed to deny chown"
fi

say "t02-chown-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t02_chown
if [[ 0 != $? ]]; then
    die "failed to predict chown"
fi

say "t02-chown-write"
SANDBOX_WRITE="${cwd}" sydbox -- ./t02_chown
if [[ 0 != $? ]]; then
    die "write didn't allow access"
fi
