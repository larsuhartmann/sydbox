include $(top_srcdir)/tests/gtest.mk

AM_CFLAGS = $(glib_CFLAGS)

TESTS += sydbox-utils

# fake out libsydbox {{{
libsydbox_SOURCES = $(top_srcdir)/src/sydbox-utils.c  \
                    $(top_srcdir)/src/sydbox-config.c \
                    $(top_srcdir)/src/path.c
AM_CFLAGS += -DSYSCONFDIR="\"$(sysconfdir)\"" -I$(top_srcdir)/src
# }}}

sydbox_utils_SOURCES = $(libsydbox_SOURCES) test-sydbox-utils.c
sydbox_utils_LDADD = $(glib_LIBS)

