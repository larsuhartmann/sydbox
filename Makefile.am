CLEANFILES= *~
MAINTAINERCLEANFILES= Makefile.in configure aclocal.m4 \
		      config.h config.h.in INSTALL
ACLOCAL_AMFLAGS= -I m4
AUTOMAKE_OPTIONS= dist-bzip2 no-dist-gzip std-options foreign
EXTRA_DIST= autogen.sh README.mkd TODO.mkd
SUBDIRS = src data . tests

check-valgrind:
	$(MAKE) -C tests/progtests check-valgrind

checksum: dist
	@echo "SHA1 $(PACKAGE)-$(VERSION).tar.bz2"
	sha1sum $(PACKAGE)-$(VERSION).tar.bz2 > $(PACKAGE)-$(VERSION).tar.bz2.sha1sum
	@echo "SIGN $(PACKAGE)-$(VERSION).tar.bz2"
	gpg --detach-sign --armor $(PACKAGE)-$(VERSION).tar.bz2

upload:
	@echo "UPLOAD $(PACKAGE)-$(VERSION).tar.bz2*"
	scp $(PACKAGE)-$(VERSION).tar.bz2* anapnea.net:www/sydbox

