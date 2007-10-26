DESTDIR=$(HOME)/.compiz/plugins

CFLAGS=`pkg-config --cflags compiz` -Wall -shared
LDFLAGS=`pkg-config --libs compiz`

PLUGIN=freewins

CC=gcc
INSTALL=install
LIBTOOL=libtool

%.lo: %.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) -c -o $@ $<

%.la: $(PLUGIN).lo
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) -rpath $(DESTDIR) -o $@ $<

all: lib$(PLUGIN).la

install: lib$(PLUGIN).la
	@mkdir -p $(DESTDIR)
	$(LIBTOOL) --mode=install $(INSTALL) lib$(PLUGIN).la \
	$(DESTDIR)/lib$(PLUGIN).la
	cp $(PLUGIN).xml $(DESTDIR)/../metadata/

clean:
	rm -rf *.lo *.o lib$(PLUGIN).* .libs *~ $(PLUGIN).schema
