##
#
# Compiz plugin Makefile
#
# Copyright : (C) 2007 by Dennis Kasprzyk
# E-mail    : onestone@deltatauchi.de
#
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
##

# plugin.info file contents
# 
# PLUGIN = foo
# PKG_DEP = pango
# LDFLAGS_ADD = -lGLU
# CFLAGS_ADD = -I/usr/include/foo
#

#load config file
include plugin.info

ifeq ($(BUILD_GLOBAL),true)
	PREFIX = $(shell pkg-config --variable=prefix compiz)
	CLIBDIR = $(shell pkg-config --variable=libdir compiz)
	CINCDIR = $(shell pkg-config --variable=includedir compiz)
	PKGDIR = $(CLIBDIR)/pkgconfig
	DESTDIR = $(shell pkg-config --variable=libdir compiz)/compiz
	XMLDIR = $(shell pkg-config --variable=prefix compiz)/share/compiz
else
	DESTDIR = $(HOME)/.compiz/plugins
	XMLDIR = $(HOME)/.compiz/metadata
endif

BUILDDIR = build

ECHO	  = `which echo`

CC        = gcc
LIBTOOL   = libtool
INSTALL   = install

BCOP      = `pkg-config --variable=bin bcop`

CFLAGS    = -g -Wall `pkg-config --cflags $(PKG_DEP) compiz ` $(CFLAGS_ADD)
LDFLAGS   = `pkg-config --libs $(PKG_DEP) compiz ` $(LDFLAGS_ADD)

POFILEDIR = $(shell if [ -n "$(PODIR)" ]; then $(ECHO) $(PODIR); else $(ECHO) ./po;fi )

is-bcop-target  := $(shell if [ -e $(PLUGIN).xml.in ]; then cat $(PLUGIN).xml.in | grep "useBcop=\"true\""; \
		     else if [ -e $(PLUGIN).xml ]; then cat $(PLUGIN).xml | grep "useBcop=\"true\""; fi; fi)

trans-target    := $(shell if [ -e $(PLUGIN).xml.in -o -e $(PLUGIN).xml ]; then $(ECHO) $(BUILDDIR)/$(PLUGIN).xml;fi )

bcop-target     := $(shell if [ -n "$(is-bcop-target)" ]; then $(ECHO) $(BUILDDIR)/$(PLUGIN).xml; fi )
bcop-target-src := $(shell if [ -n "$(is-bcop-target)" ]; then $(ECHO) $(BUILDDIR)/$(PLUGIN)_options.c; fi )
bcop-target-hdr := $(shell if [ -n "$(is-bcop-target)" ]; then $(ECHO) $(BUILDDIR)/$(PLUGIN)_options.h; fi )

gen-schemas     := $(shell if [ -e $(PLUGIN).xml.in -o -e $(PLUGIN).xml -a -n "`pkg-config --variable=xsltdir compiz-gconf`" ]; then $(ECHO) true; fi )
schema-target   := $(shell if [ -n "$(gen-schemas)" ]; then $(ECHO) $(BUILDDIR)/$(PLUGIN).xml; fi )
schema-output   := $(shell if [ -n "$(gen-schemas)" ]; then $(ECHO) $(BUILDDIR)/compiz-$(PLUGIN).schema; fi )

ifeq ($(BUILD_GLOBAL),true)
    pkg-target         := $(shell if [ -e compiz-$(PLUGIN).pc.in -a -n "$(PREFIX)" -a -d "$(PREFIX)" ]; then $(ECHO) "$(BUILDDIR)/compiz-$(PLUGIN).pc"; fi )
    hdr-install-target := $(shell if [ -e compiz-$(PLUGIN).pc.in -a -n "$(PREFIX)" -a -d "$(PREFIX)" -a -e $(PLUGIN).h ]; then $(ECHO) "$(PLUGIN).h"; fi )
endif

# find all the object files (including those from .moc.cpp files)

c-objs     := $(patsubst %.c,%.lo,$(shell find -name '*.c' 2> /dev/null | grep -v "$(BUILDDIR)/" | sed -e 's/^.\///'))
c-objs     := $(filter-out $(bcop-target-src:.c=.lo),$(c-objs))

h-files    := $(shell find -name '*.h' 2> /dev/null | grep -v "$(BUILDDIR)/" | sed -e 's/^.\///')
h-files    += $(bcop-target-hdr)

all-c-objs := $(addprefix $(BUILDDIR)/,$(c-objs)) 
all-c-objs += $(bcop-target-src:.c=.lo)

# system include path parameter, -isystem doesn't work on old gcc's
inc-path-param = $(shell if [ -z "`gcc --version | head -n 1 | grep ' 3'`" ]; then $(ECHO) "-isystem"; else $(ECHO) "-I"; fi)

# default color settings
color := $(shell if [ $$TERM = "dumb" ]; then $(ECHO) "no"; else $(ECHO) "yes"; fi)

#
# Do it.
#

.PHONY: $(BUILDDIR) build-dir trans-target bcop-build pkg-creation schema-creation c-build-objs c-link-plugin

all: $(BUILDDIR) build-dir trans-target bcop-build pkg-creation schema-creation c-build-objs c-link-plugin

trans-build: $(trans-target)

bcop-build:   $(bcop-target-hdr) $(bcop-target-src)

schema-creation: $(schema-output)

c-build-objs: $(all-c-objs)

c-link-plugin: $(BUILDDIR)/lib$(PLUGIN).la

pkg-creation: $(pkg-target)

#
# Create build directory
#

$(BUILDDIR) :
	@mkdir -p $(BUILDDIR)

$(DESTDIR) :
	@mkdir -p $(DESTDIR)

#
# fallback if xml.in doesn't exists
#
$(BUILDDIR)/%.xml: %.xml
	@cp $< $@

#
# Translating
#
$(BUILDDIR)/%.xml: %.xml.in
	@if [ -d $(POFILEDIR) ]; then \
		if [ '$(color)' != 'no' ]; then \
			$(ECHO) -e -n "\033[0;1;5mtranslate \033[0m: \033[0;32m$< \033[0m-> \033[0;31m$@\033[0m"; \
		else \
			$(ECHO) "translate $<  ->  $@"; \
		fi; \
		intltool-merge -x -u $(POFILEDIR) $< $@ > /dev/null; \
		if [ '$(color)' != 'no' ]; then \
			$(ECHO) -e "\r\033[0mtranslate : \033[34m$< -> $@\033[0m"; \
		fi; \
	else \
		if [ '$(color)' != 'no' ]; then \
			$(ECHO) -e -n "\033[0;1;5mconvert   \033[0m: \033[0;32m$< \033[0m-> \033[0;31m$@\033[0m"; \
		else \
			$(ECHO) "convert   $<  ->  $@"; \
		fi; \
		cat $< | sed -e 's;<_;<;g' -e 's;</_;</;g' > $@; \
		if [ '$(color)' != 'no' ]; then \
			$(ECHO) -e "\r\033[0mconvert   : \033[34m$< -> $@\033[0m"; \
		fi; \
	fi

#
# BCOP'ing

$(BUILDDIR)/%_options.h: $(BUILDDIR)/%.xml
	@if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e -n "\033[0;1;5mbcop'ing  \033[0m: \033[0;32m$< \033[0m-> \033[0;31m$@\033[0m"; \
	else \
		$(ECHO) "bcop'ing  $<  ->  $@"; \
	fi
	@$(BCOP) --header=$@ $<
	@if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e "\r\033[0mbcop'ing  : \033[34m$< -> $@\033[0m"; \
	fi

$(BUILDDIR)/%_options.c: $(BUILDDIR)/%.xml
	@if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e -n "\033[0;1;5mbcop'ing  \033[0m: \033[0;32m$< \033[0m-> \033[0;31m$@\033[0m"; \
	else \
		$(ECHO) "bcop'ing  $<  ->  $@"; \
	fi
	@$(BCOP) --source=$@ $< 
	@if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e "\r\033[0mbcop'ing  : \033[34m$< -> $@\033[0m"; \
	fi

#
# Schema generation

$(BUILDDIR)/compiz-%.schema: $(BUILDDIR)/%.xml
	@if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e -n "\033[0;1;5mschema'ing\033[0m: \033[0;32m$< \033[0m-> \033[0;31m$@\033[0m"; \
	else \
		$(ECHO) "schema'ing  $<  ->  $@"; \
	fi
	@xsltproc `pkg-config --variable=xsltdir compiz-gconf`/schemas.xslt $< > $@
	@if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e "\r\033[0mschema    : \033[34m$< -> $@\033[0m"; \
	fi

#
# pkg config file generation

$(BUILDDIR)/compiz-%.pc: compiz-%.pc.in
	@if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e -n "\033[0;1;5mpkgconfig \033[0m: \033[0;32m$< \033[0m-> \033[0;31m$@\033[0m"; \
	else \
		$(ECHO) "pkgconfig   $<  ->  $@"; \
	fi
	@COMPIZREQUIRES=`cat $(PKGDIR)/compiz.pc | grep Requires | sed -e 's;Requires: ;;g'`; \
    COMPIZCFLAGS=`cat $(PKGDIR)/compiz.pc | grep Cflags | sed -e 's;Cflags: ;;g'`; \
    sed -e 's;@prefix@;$(PREFIX);g' -e 's;\@libdir@;$(CLIBDIR);g' \
        -e 's;@includedir@;$(CINCDIR);g' -e 's;\@VERSION@;0.0.1;g' \
        -e "s;@COMPIZ_REQUIRES@;$$COMPIZREQUIRES;g" \
        -e "s;@COMPIZ_CFLAGS@;$$COMPIZCFLAGS;g" $< > $@;
	@if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e "\r\033[0mpkgconfig : \033[34m$< -> $@\033[0m"; \
	fi

#
# Compiling
#

$(BUILDDIR)/%.lo: %.c $(h-files)
	@if [ '$(color)' != 'no' ]; then \
		$(ECHO) -n -e "\033[0;1;5mcompiling \033[0m: \033[0;32m$< \033[0m-> \033[0;31m$@\033[0m"; \
	else \
		$(ECHO) "compiling $< -> $@"; \
	fi
	@$(LIBTOOL) --quiet --mode=compile $(CC) $(CFLAGS) -I$(BUILDDIR) -c -o $@ $<
	@if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e "\r\033[0mcompiling : \033[34m$< -> $@\033[0m"; \
	fi

$(BUILDDIR)/%.lo: $(BUILDDIR)/%.c $(h-files)
	@if [ '$(color)' != 'no' ]; then \
		$(ECHO) -n -e "\033[0;1;5mcompiling \033[0m: \033[0;32m$< \033[0m-> \033[0;31m$@\033[0m"; \
	else \
		$(ECHO) "compiling $< -> $@"; \
	fi
	@$(LIBTOOL) --quiet --mode=compile $(CC) $(CFLAGS) -I$(BUILDDIR) -c -o $@ $<
	@if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e "\r\033[0mcompiling : \033[34m$< -> $@\033[0m"; \
	fi


#
# Linking
#

cxx-rpath-prefix := -Wl,-rpath,

$(BUILDDIR)/lib$(PLUGIN).la: $(all-c-objs)
	@if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e -n "\033[0;1;5mlinking   \033[0m: \033[0;31m$@\033[0m"; \
	else \
		$(ECHO) "linking   : $@"; \
	fi
	@$(LIBTOOL) --quiet --mode=link $(CC) $(LDFLAGS) -rpath $(DESTDIR) -o $@ $(all-c-objs)
	@if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e "\r\033[0mlinking   : \033[34m$@\033[0m"; \
	fi


clean:
	@if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e -n "\033[0;1;5mremoving  \033[0m: \033[0;31m./$(BUILDDIR)\033[0m"; \
	else \
		$(ECHO) "removing  : ./$(BUILDDIR)"; \
	fi
	@rm -rf $(BUILDDIR)
	@if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e "\r\033[0mremoving  : \033[34m./$(BUILDDIR)\033[0m"; \
	fi
	

install: $(DESTDIR) all
	@if [ '$(color)' != 'no' ]; then \
	    $(ECHO) -n -e "\033[0;1;5minstall   \033[0m: \033[0;31m$(DESTDIR)/lib$(PLUGIN).so\033[0m"; \
	else \
	    $(ECHO) "install   : $(DESTDIR)/lib$(PLUGIN).so"; \
	fi
	@mkdir -p $(DESTDIR)
	@$(INSTALL) $(BUILDDIR)/.libs/lib$(PLUGIN).so $(DESTDIR)/lib$(PLUGIN).so
	@if [ '$(color)' != 'no' ]; then \
	    $(ECHO) -e "\r\033[0minstall   : \033[34m$(DESTDIR)/lib$(PLUGIN).so\033[0m"; \
	fi
	@if [ -e $(BUILDDIR)/$(PLUGIN).xml ]; then \
	    if [ '$(color)' != 'no' ]; then \
		$(ECHO) -n -e "\033[0;1;5minstall   \033[0m: \033[0;31m$(XMLDIR)/$(PLUGIN).xml\033[0m"; \
	    else \
		$(ECHO) "install   : $(XMLDIR)/$(PLUGIN).xml"; \
	    fi; \
	    mkdir -p $(XMLDIR); \
	    cp $(BUILDDIR)/$(PLUGIN).xml $(XMLDIR)/$(PLUGIN).xml; \
	    if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e "\r\033[0minstall   : \033[34m$(XMLDIR)/$(PLUGIN).xml\033[0m"; \
	    fi; \
	fi
	@if [ -n "$(hdr-install-target)" ]; then \
	    if [ '$(color)' != 'no' ]; then \
		$(ECHO) -n -e "\033[0;1;5minstall   \033[0m: \033[0;31m$(CINCDIR)/compiz/$(hdr-install-target)\033[0m"; \
	    else \
		$(ECHO) "install   : $(CINCDIR)/compiz/$(hdr-install-target)"; \
	    fi; \
	    cp $(hdr-install-target) $(CINCDIR)/compiz/$(hdr-install-target); \
	    if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e "\r\033[0minstall   : \033[34m$(CINCDIR)/compiz/$(hdr-install-target)\033[0m"; \
	    fi; \
	fi
	@if [ -n "$(pkg-target)" ]; then \
	    if [ '$(color)' != 'no' ]; then \
		$(ECHO) -n -e "\033[0;1;5minstall   \033[0m: \033[0;31m$(PKGDIR)/compiz-$(PLUGIN).pc\033[0m"; \
	    else \
		$(ECHO) "install   : $(PKGDIR)/compiz-$(PLUGIN).pc"; \
	    fi; \
	    cp $(pkg-target) $(PKGDIR)/compiz-$(PLUGIN).pc; \
	    if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e "\r\033[0minstall   : \033[34m$(PKGDIR)/compiz-$(PLUGIN).pc\033[0m"; \
	    fi; \
	fi
	@if [ -n "$(schema-output)" -a -e "$(schema-output)" ]; then \
	    if [ '$(color)' != 'no' ]; then \
		$(ECHO) -n -e "\033[0;1;5minstall   \033[0m: \033[0;31m$(schema-output)\033[0m"; \
	    else \
		$(ECHO) "install   : $(schema-output)"; \
	    fi; \
	    gconftool-2 --install-schema-file=$(schema-output) > /dev/null; \
	    if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e "\r\033[0minstall   : \033[34m$(schema-output)\033[0m"; \
	    fi; \
	fi

uninstall:	
	@if [ -e $(DESTDIR)/lib$(PLUGIN).so ]; then \
	    if [ '$(color)' != 'no' ]; then \
		$(ECHO) -n -e "\033[0;1;5muninstall \033[0m: \033[0;31m$(DESTDIR)/lib$(PLUGIN).so\033[0m"; \
	    else \
		$(ECHO) "uninstall : $(DESTDIR)/lib$(PLUGIN).so"; \
	    fi; \
	    rm -f $(DESTDIR)/lib$(PLUGIN).so; \
	    if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e "\r\033[0muninstall : \033[34m$(DESTDIR)/lib$(PLUGIN).so\033[0m"; \
	    fi; \
	fi
	@if [ -e $(XMLDIR)/$(PLUGIN).xml ]; then \
	    if [ '$(color)' != 'no' ]; then \
		$(ECHO) -n -e "\033[0;1;5muninstall \033[0m: \033[0;31m$(XMLDIR)/$(PLUGIN).xml\033[0m"; \
	    else \
		$(ECHO) "uninstall : $(XMLDIR)/$(PLUGIN).xml"; \
	    fi; \
	    rm -f $(XMLDIR)/$(PLUGIN).xml; \
	    if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e "\r\033[0muninstall : \033[34m$(XMLDIR)/$(PLUGIN).xml\033[0m"; \
	    fi; \
	fi
	@if [ -n "$(hdr-install-target)" -a -e $(CINCDIR)/compiz/$(hdr-install-target) ]; then \
	    if [ '$(color)' != 'no' ]; then \
		$(ECHO) -n -e "\033[0;1;5muninstall \033[0m: \033[0;31m$(CINCDIR)/compiz/$(hdr-install-target)\033[0m"; \
	    else \
		$(ECHO) "uninstall : $(CINCDIR)/compiz/$(hdr-install-target)"; \
	    fi; \
	    rm -f $(CINCDIR)/compiz/$(hdr-install-target); \
	    if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e "\r\033[0muninstall : \033[34m$(CINCDIR)/compiz/$(hdr-install-target)\033[0m"; \
	    fi; \
	fi
	@if [ -n "$(pkg-target)" -a -e $(PKGDIR)/compiz-$(PLUGIN).pc ]; then \
	    if [ '$(color)' != 'no' ]; then \
		$(ECHO) -n -e "\033[0;1;5muninstall \033[0m: \033[0;31m$(PKGDIR)/compiz-$(PLUGIN).pc\033[0m"; \
	    else \
		$(ECHO) "uninstall : $(PKGDIR)/compiz-$(PLUGIN).pc"; \
	    fi; \
	    rm -f $(PKGDIR)/compiz-$(PLUGIN).pc; \
	    if [ '$(color)' != 'no' ]; then \
		$(ECHO) -e "\r\033[0muninstall : \033[34m$(PKGDIR)/compiz-$(PLUGIN).pc\033[0m"; \
	    fi; \
	fi
