SUBDIRS = . progtests

TESTS = check_sydbox
check_PROGRAMS = check_sydbox

check_sydbox_SOURCES = check_children.c check_path.c check_trace.c \
		       check_util.c check_sydbox.h check_sydbox.c \
		       $(top_builddir)/src/children.c \
		       $(top_builddir)/src/util.c $(top_builddir)/src/context.c \
		       $(top_builddir)/src/path.c $(top_builddir)/src/syscall.c \
		       $(top_builddir)/src/trace.c $(top_builddir)/src/wrappers.c \
		       $(top_builddir)/src/proc.c $(top_builddir)/src/syscall_marshaller.c \
		       $(top_builddir)/src/sydbox-log.c $(top_builddir)/src/sydbox-config.c

check_sydbox_CFLAGS = @SYDBOX_CFLAGS@ -DSYSCONFDIR=\"$(sysconfdir)\" $(glib_CFLAGS) $(confuse_CFLAGS) $(check_CFLAGS)
check_sydbox_LDADD = $(glib_LIBS) $(gobject_LIBS) $(confuse_LIBS) $(check_LIBS)

