#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

start_test "t39-magic-unpredict-stat"
sydbox -- bash <<EOF
[[ -e /dev/sydbox/unpredict ]]
EOF
if [[ 0 != $? ]]; then
    die "/dev/sydbox/unpredict doesn't exist"
fi
end_test

start_test "t39-magic-unpredict-locked"
sydbox --lock -- bash <<EOF
[[ -e /dev/sydbox/unpredict ]]
EOF
if [[ 0 == $? ]]; then
    die "/dev/sydbox/unpredict exists"
fi
end_test

start_test "t39-magic-unpredict-remove"
sydbox -- bash <<EOF
:>/dev/sydbox/predict/${cwd}
:>/dev/sydbox/unpredict/${cwd}
echo Oh Arnold Layne, its not the same > arnold.layne
EOF
if [[ 0 == $? ]]; then
    die "failed to remove prefix using /dev/sydbox/unpredict"
elif [[ -n "$(< arnold.layne)" ]]; then
    die "file not empty, failed to remove prefix using /dev/sydbox/unpredict"
fi
end_test
