#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

start_test "t15-mount-deny"
sydbox -- ./t15_mount
if [[ 0 == $? ]]; then
    die "failed to deny mount"
fi
end_test

