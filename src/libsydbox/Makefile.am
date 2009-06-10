AM_CFLAGS= -DSYDBOX_I_KNOW_THIS_IS_UNSTABLE=1 \
	   -DSYSCONFDIR=\"$(sysconfdir)\" -DGIT_HEAD=\"$(GIT_HEAD)\" \
	   $(glib_CFLAGS) $(gobject_CFLAGS) @SYDBOX_CFLAGS@

lib_LTLIBRARIES= libsydbox.la
libsydbox_la_SOURCES= sydbox-unstable.h sydbox.h \
		      sydbox-context.c sydbox-context.h
libsydbox_la_LIBADD= $(glib_LIBS) $(gobject_LIBS)
