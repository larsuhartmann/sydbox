#!/bin/sh
# vim: set sw=4 et sts=4 tw=80 :

die() {
    echo "$@" >&2
    exit 1
}

rm -f config.cache
aclocal || die "aclocal failed"
autoheader || die "autoheader failed"
autoconf || die "autoconf failed"
automake -a --copy || die "automake failed"
