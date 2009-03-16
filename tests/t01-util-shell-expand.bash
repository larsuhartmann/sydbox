#!/bin/bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

str="${HOME}/see.emily.play"
exstr="$(./t01_util_shell_expand '${HOME}/see.emily.play')"
if [[ "$str" != "$exstr" ]]; then
    echo "'$str' != '$exstr'" >&2
    exit 1
fi

str="/dev/sydbox/predict"
exstr="$(./t01_util_shell_expand '$(echo -n /dev/sydbox)/predict')"
if [[ "$str" != "$exstr" ]]; then
    echo "'$str' != '$exstr'" >&2
    exit 1
fi
