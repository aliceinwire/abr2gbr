# GIMP Text Colorer GNUMakefile
# Copyright (C) 2000 Marco Lamberto <lm@sunnyspot.org>
# Web page: http://the.sunnyspot.org/gimp/
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

CC				= gcc
CDEBUG		= $(DEBUG) -g #-DDEBUG
CCFLAGS		= -Wall -Wstrict-prototypes -Wmissing-declarations -I. $(CDEBUG)
CLIBS			=
SOURCES		= $(wildcard *.c)

OBJDIR		= .obj
BINDIR		=	/usr/bin
CFLAGS		= $(CCFLAGS) $(shell $(BINDIR)/glib-config --cflags)
LIBS			= $(CLIBS) $(shell $(BINDIR)/glib-config --libs)
DESTS			= $(addprefix $(OBJDIR)/, $(SOURCES:.c=.o))
BIN				= abr2gbr
DEPEND		= $(OBJDIR)/.depend


default: all


$(OBJDIR):
	if [ ! -d $@ ]; then mkdir $@; fi


all: $(OBJDIR) $(OBJDIR)/$(BIN)

$(OBJDIR)/%.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<

$(OBJDIR)/$(BIN): $(DESTS)
	$(CC) -o $@ $(CFLAGS) $(LIBS) $(DESTS)
	if [ ! -L $(BIN) ]; then \
		ln -s $(OBJDIR)/$(BIN) .; \
		ln -s $(OBJDIR)/$(BIN) jbr2gbr; \
	fi

dep depend:
	$(CC) $(CFLAGS) -M $(SOURCES) > $(DEPEND)

install: $(BIN)
	strip $(BIN)


clean:
	-rm -f core $(DESTS) \
		$(OBJDIR)/$(BIN) $(BIN) $(BIN)-ui \
		$(DEPEND) \
		font_selection jbr2gbr
	-rmdir $(OBJDIR)

cleandests:
	-rm -f core $(DESTS)

# vim: set ts=2 sw=2 tw=79 ai nowrap:
