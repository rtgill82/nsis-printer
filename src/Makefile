#
# Created:  Fri 12 Dec 2014 07:37:55 PM PST
# Modified: Tue 13 Sep 2016 10:43:43 PM PDT
#

DLLNAME = Printer.dll
CCPATH  = /usr/bin
HOST = i686-w64-mingw32
CC = $(CCPATH)/$(HOST)-gcc
RC = $(CCPATH)/$(HOST)-windres

CFLAGS = -march=i686 -fomit-frame-pointer -ftracer -Os -Wall
EXTRA_LDFLAGS := $(LDFLAGS)
override LDFLAGS := -s $(EXTRA_LDFLAGS)

ifeq ($(DEBUG),true)
CFLAGS  = -march=i686 -g3 -Og -Wall $(EXTRA_CFLAGS)
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
