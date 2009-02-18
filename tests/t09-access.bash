#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

chmod +x arnold.layne
if [[ 0 != $? ]]; then
    die "chmod +x arnold.layne"
fi

start_test "t03-access-r_ok-allow"
sydbox -- ./t09_access 0
if [[ 0 != $? ]]; then
    die "denied access for access(\"arnold.layne\", R_OK)"
fi
end_test

start_test "t03-access-w_ok-deny"
sydbox -- ./t09_access 1
if [[ 0 == $? ]]; then
    die "allowed access for access(\"arnold.layne\", W_OK)"
fi
end_test

start_test "t03-access-w_ok-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t09_access 1
if [[ 0 != $? ]]; then
    die "failed to predict access(\"arnold.layne\", W_OK)"
fi
end_test

start_test "t03-access-w_ok-write"
SANDBOX_WRITE="${cwd}" sydbox -- ./t09_access 1
if [[ 0 != $? ]]; then
    die "failed to allow access(\"arnold.layne\", W_OK)"
fi
end_test

start_test "t03-access-x_ok-allow"
sydbox -- ./t09_access 2
if [[ 0 != $? ]]; then
    die "denied access for access(\"arnold.layne\", X_OK)"
fi
end_test
