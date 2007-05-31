DESTDIR = $(HOME)/.compiz/plugins
METADATADIR = $(HOME)/.compiz/metadata
IMAGEDIR = $(HOME)/.compiz/images

XSLTDIR = `pkg-config --variable=xsltdir compiz-gconf`

PLUGIN  = notification

CC      = gcc
LIBTOOL = libtool
INSTALL = install

CFLAGS  = -Wall -g `pkg-config --cflags compiz gtk+-2.0 glib-2.0 ` -shared
LDFLAGS = `pkg-config --libs compiz libnotify`

%.lo: %.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) -c -o $@ $<

%.la: $(PLUGIN).lo
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) -rpath $(DESTDIR) -o $@ $<

all: lib$(PLUGIN).la

install: lib$(PLUGIN).la
	@mkdir -p $(DESTDIR)
	$(LIBTOOL) --mode=install $(INSTALL) lib$(PLUGIN).la \
	$(DESTDIR)/lib$(PLUGIN).la
	@mkdir -p $(METADATADIR)
	@mkdir -p $(IMAGEDIR)
	@cp $(PLUGIN).xml $(METADATADIR)
	@cp images/compiz.png $(IMAGEDIR)

install-schema:
	@xsltproc -o $(PLUGIN).schema $(XSLTDIR)/schemas.xslt $(PLUGIN).xml
	@gconftool-2 --install-schema-file=$(PLUGIN).schema

clean:
	rm -rf *.lo *.o lib$(PLUGIN).* .libs *~ $(PLUGIN).schema

