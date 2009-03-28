SUBDIRS = . progtests

TESTS = check_sydbox
check_PROGRAMS = check_sydbox

check_sydbox_SOURCES = check_children.c check_path.c check_trace.c check_syscall.c \
		       check_util.c check_sydbox.h check_sydbox.c \
		       $(top_builddir)/src/defs.h $(top_builddir)/src/children.c \
		       $(top_builddir)/src/util.c $(top_builddir)/src/context.c \
		       $(top_builddir)/src/path.c $(top_builddir)/src/syscall.c \
		       $(top_builddir)/src/trace.c $(top_builddir)/src/wrappers.c \
		       $(top_builddir)/src/proc.c $(top_builddir)/src/getcwd.c \
		       $(top_builddir)/src/log.c

check_sydbox_CFLAGS = @SYDBOX_CFLAGS@ $(glib_CFLAGS) $(check_CFLAGS)
check_sydbox_LDADD = $(glib_LIBS) $(check_LIBS)

