#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

no_create_files=1
. ./test-lib.bash

start_test "ctl-enabled"
sydbox -- "$sydboxctl" enabled
if [[ 0 != $? ]]; then
    die "sydboxctl enabled failed!"
fi
end_test
