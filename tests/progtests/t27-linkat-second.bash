#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

old_umask=$(umask)
umask 022
touch see.emily.play/gnome
if [[ 0 != $? ]]; then
    die "touch see.emily.play/gnome"
elif [[ ! -f arnold.layne ]]; then
    die "touch see.emily.play/gnome (no file)"
fi
umask $old_umask

start_test "t27-linkat-second-deny"
SANDBOX_WRITE="${cwd}/see.emily.play" sydbox -- ./t27_linkat_second
if [[ 0 == $? ]]; then
    die "failed to deny linkat"
elif [[ -f jugband.blues ]]; then
    die "file exists, failed to deny linkat"
fi
end_test

start_test "t27-linkat-second-predict"
SANDBOX_WRITE="${cwd}/see.emily.play" SANDBOX_PREDICT="${cwd}" sydbox -- ./t27_linkat_second
if [[ 0 != $? ]]; then
    die "failed to predict linkat"
elif [[ -f jugband.blues ]]; then
    die "predict allowed access"
fi
end_test

start_test "t27-linkat-second-write"
SANDBOX_WRITE="${cwd}" sydbox -- ./t27_linkat_second
if [[ 0 != $? ]]; then
    die "failed to allow linkat"
elif [[ ! -f jugband.blues ]]; then
    die "file doesn't exist, failed to allow linkat"
fi
end_test
