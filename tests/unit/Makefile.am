include $(top_srcdir)/tests/gtest.mk

AM_CFLAGS = $(glib_CFLAGS)

UNIT_TESTS = sydbox-utils children path

# fake out libsydbox {{{
libsydbox_SOURCES = $(top_srcdir)/src/sydbox-utils.c  \
                    $(top_srcdir)/src/sydbox-config.c \
                    $(top_srcdir)/src/path.c          \
                    $(top_srcdir)/src/children.c
AM_CFLAGS += -DSYSCONFDIR="\"$(sysconfdir)\"" -I$(top_srcdir)/src
# }}}

TESTS += $(UNIT_TESTS)

noinst_PROGRAMS = $(UNIT_TESTS)

sydbox_utils_SOURCES = $(libsydbox_SOURCES) test-sydbox-utils.c
sydbox_utils_LDADD = $(glib_LIBS)

children_SOURCES = $(libsydbox_SOURCES) test-children.c
children_LDADD = $(glib_LIBS)

path_SOURCES = $(libsydbox_SOURCES) test-path.c
path_LDADD = $(glib_LIBS)

