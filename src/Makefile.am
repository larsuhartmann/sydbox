AM_CFLAGS= -DSYSCONFDIR=\"$(sysconfdir)\" -DGITHEAD=\"$(GITHEAD)\" $(confuse_CFLAGS) @SYDBOX_CFLAGS@
bin_PROGRAMS = sydbox
sydbox_SOURCES = defs.h children.c util.c context.c path.c \
		 realpath.c syscall.c trace.c main.c
sydbox_LDADD= $(confuse_LIBS)

check:
	make -C $(top_builddir)/tests check
