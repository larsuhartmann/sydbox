#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

start_test "t36-magic-write-locked"
sydbox --lock -- bash <<EOF
[[ -e /dev/sydbox/write ]]
EOF
if [[ 0 == $? ]]; then
    die "/dev/sydbox/write exists"
fi
end_test

start_test "t36-magic-write-add"
sydbox -- bash <<EOF
[[ -e /dev/sydbox/write/${cwd} ]]
echo Oh Arnold Layne, its not the same > arnold.layne
EOF
if [[ 0 != $? ]]; then
    die "failed to add prefix using /dev/sydbox/write"
elif [[ -z "$(< arnold.layne)" ]]; then
    die "file empty, failed to add prefix using /dev/sydbox/write"
fi
end_test
