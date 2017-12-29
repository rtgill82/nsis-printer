#
# Created:  Fri 12 Dec 2014 07:37:55 PM PST
# Modified: Fri 29 Dec 2017 12:47:26 PM PST
#
# Copyright 2016,2017 (C) Robert Gill
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

DLLNAME = Printer.dll
CCPATH  = /usr/bin
HOST = i686-w64-mingw32
CC = $(CCPATH)/$(HOST)-gcc
RC = $(CCPATH)/$(HOST)-windres

CFLAGS = -Wall -march=i686 -Os -fomit-frame-pointer
EXTRA_LDFLAGS := $(LDFLAGS)
override LDFLAGS := -s $(EXTRA_LDFLAGS)

ifeq ($(DEBUG),true)
CFLAGS  = -Wall -march=i686 -Og -g3 $(EXTRA_CFLAGS)
override LDFLAGS  =
override LDFLAGS := $(EXTRA_LDFLAGS)
endif

CPPFLAGS += -DNSISCALL="__attribute__((__stdcall__))"
override LDFLAGS += -mwindows -Wl,--kill-at -static-libgcc -shared
UNICODE_CPPFLAGS = -DUNICODE -D_UNICODE
UNICODE_LDFLAGS  = -municode

OBJECTS = pluginapi.o printer.o printer.res
LDLIBS = -lwinspool

ANSI_OUTDIR = ../nsis/plugins/x86-ansi
ANSI_DLL = $(ANSI_OUTDIR)/$(DLLNAME)
ANSI_OBJDIR = .obj/ansi
ANSI_OBJECTS = $(foreach O,$(OBJECTS),$(ANSI_OBJDIR)/$O)

UNICODE_OUTDIR = ../nsis/plugins/x86-unicode
UNICODE_DLL = $(UNICODE_OUTDIR)/$(DLLNAME)
UNICODE_OBJDIR = .obj/unicode
UNICODE_OBJECTS = $(foreach O,$(OBJECTS),$(UNICODE_OBJDIR)/$O)

.SUFFIXES: .rc .res

all: ansi unicode

ansi: $(ANSI_DLL)

unicode: $(UNICODE_DLL)

$(ANSI_DLL): $(ANSI_OBJECTS)
	$(CC) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(UNICODE_DLL): $(UNICODE_OBJECTS)
	$(CC) $^ $(UNICODE_LDFLAGS) $(LDFLAGS) $(LDLIBS) -o $@

$(ANSI_OBJDIR)/%.o: %.c | .obj/ansi
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(ANSI_OBJDIR)/%.res: %.rc | .obj/ansi
	$(RC) $(CPPFLAGS) -O coff -o $@ $<

$(UNICODE_OBJDIR)/%.o: %.c | .obj/unicode
	$(CC) $(CFLAGS) $(CPPFLAGS) $(UNICODE_CPPFLAGS) -c $< -o $@

$(UNICODE_OBJDIR)/%.res: %.rc | .obj/unicode
	$(RC) $(CPPFLAGS) $(UNICODE_CPPFLAGS) -O coff -o $@ $<

.obj/ansi:
	-mkdir -p .obj/ansi

.obj/unicode:
	-mkdir -p .obj/unicode

clean:
	-rm $(ANSI_DLL) $(UNICODE_DLL)
	-rm -r .obj

.PHONY: all clean ansi unicode
