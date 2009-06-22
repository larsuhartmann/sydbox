#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

start_test "t38-magic-predict-stat"
sydbox -- bash <<EOF
[[ -e /dev/sydbox/predict ]]
EOF
if [[ 0 != $? ]]; then
    die "/dev/sydbox/predict doesn't exist"
fi
end_test

start_test "t38-magic-predict-locked"
sydbox --lock -- bash <<EOF
[[ -e /dev/sydbox/predict ]]
EOF
if [[ 0 == $? ]]; then
    die "/dev/sydbox/predict exists"
fi
end_test

start_test "t38-magic-predict-add"
sydbox -- bash <<EOF
:>/dev/sydbox/predict/${cwd}
echo Oh Arnold Layne, its not the same > arnold.layne
EOF
if [[ 0 != $? ]]; then
    die "failed to add prefix using /dev/sydbox/predict"
elif [[ -n "$(< arnold.layne)" ]]; then
    die "file not empty, predict allowed access"
fi
end_test
