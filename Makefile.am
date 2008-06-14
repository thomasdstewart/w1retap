SUBDIRS = src
EXTRA_DIST = docs autogen.sh contrib

dist-hook:
	rm -rf `find $(distdir) -name .svn`

install-data-local:
	@$(mkinstalldirs) $(DESTDIR)$(docdir)
	@$(mkinstalldirs) $(DESTDIR)$(docdir)/html
	@for f in docs/*.pdf docs/*.txt docs/*.sql docs/README* docs/ds2490* docs/*.odt ; do \
		$(INSTALL_DATA) $$f $(DESTDIR)$(docdir) ; \
	done
	@for f in docs/*.pdf docs/*.odt ; do \
		$(INSTALL_DATA) $$f $(DESTDIR)$(docdir) ; \
	done
