EXTRA_DIST= sydbox.1.txt sydbox.1.xml sydbox.1
BUILT_SOURCES= sydbox.1

MANPAGE_XSL:=/etc/asciidoc/docbook-xsl/manpage.xsl
all: sydbox.1
sydbox.1.xml: sydbox.1.txt
	asciidoc -d manpage -b docbook $<
sydbox.1: sydbox.1.xml
	xsltproc --nonet "$(MANPAGE_XSL)" $<
