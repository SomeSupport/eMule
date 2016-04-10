# Copyright (C) 1999 Scott Thomas Haug <scott@id3.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

EXTRA_DIST =            \
  Doxyfile              \
  Doxyfile.in           \
  3d-logo.gif           \
  attilas_id3logo.jpg   \
  fillpx.gif            \
  id3guide.html         \
  id3lib-manual.php     \
  id3lib.css            \
  id3v2.3.0.html        \
  id3v2.3.0.txt         \
  id3v2.gif             \
  id3v2.ico             \
  id3v2.png             \
  index.html            \
  index.html.in         \
  musicmatch.txt

Doxyfile: $(srcdir)/Doxyfile.in  $(top_builddir)/config.status
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status

index.html: $(srcdir)/index.html.in  $(top_builddir)/config.status
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status

docsdistdir = $(PACKAGE)-doc-$(VERSION)

dox: Doxyfile
	-rm -rf @DOX_DIR_HTML@ @DOX_DIR_LATEX@
	cd $(srcdir) && doxygen Doxyfile

docs: dox index.html id3lib.css
	-rm -rf $(docsdistdir).*
	cd $(srcdir)/@DOX_DIR_LATEX@ && \
	make pdf && \
	(cp *.pdf ../$(docsdistdir).pdf || true) && \
	cp *.ps ../$(docsdistdir).ps && \
	gzip -9 ../$(docsdistdir).ps
	-rm -rf $(srcdir)/@DOX_DIR_LATEX@

