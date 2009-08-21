SUBDIRS = . progtests unit

TESTS = check_sydbox
check_PROGRAMS = check_sydbox

check_sydbox_SOURCES = check_trace.c \
		       check_sydbox.h check_sydbox.c \
		       $(top_builddir)/src/children.c \
		       $(top_builddir)/src/context.c \
		       $(top_builddir)/src/path.c $(top_builddir)/src/syscall.c \
		       $(top_builddir)/src/trace.c $(top_builddir)/src/wrappers.c \
		       $(top_builddir)/src/proc.c \
		       $(top_builddir)/src/sydbox-log.c $(top_builddir)/src/sydbox-config.c \
		       $(top_builddir)/src/sydbox-utils.c \
		       $(top_builddir)/src/hooker.c \
		       $(top_builddir)/src/trace-util.c
# dispatch.c
check_sydbox_SOURCES+= $(top_builddir)/src/dispatch.h $(top_builddir)/src/dispatch-table.h
if I386
check_sydbox_SOURCES+= $(top_builddir)/src/dispatch.c $(top_builddir)/src/trace-x86.c
endif
if X86_64
check_sydbox_SOURCES+= $(top_builddir)/src/dispatch32.c $(top_builddir)/src/dispatch64.c \
		       $(top_builddir)/src/trace-x86_64.c
endif
if IA64
check_sydbox_SOURCES+= $(top_builddir)/src/dispatch.c $(top_builddir)/src/trace-ia64.c
endif
if POWERPC
check_sydbox_SOURCES+= $(top_builddir)/src/dispatch.c $(top_builddir)/src/trace-powerpc.c
endif

nodist_check_sydbox_SOURCES= $(top_builddir)/src/syscall_marshaller.c

check_sydbox_CFLAGS = \
		      -I$(top_builddir)/src \
		      @SYDBOX_CFLAGS@ \
		      -DSYSCONFDIR=\"$(sysconfdir)\" \
		      $(glib_CFLAGS) $(gobject_CFLAGS) $(check_CFLAGS)
check_sydbox_LDADD = $(glib_LIBS) $(gobject_LIBS) $(check_LIBS)

check-valgrind:
	$(MAKE) -C progtests check-valgrind
