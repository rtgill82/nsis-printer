CFLAGS  = -march=i686 -fomit-frame-pointer -ftracer -Os -Wall
HOST    = i686-w64-mingw32
LDFLAGS = -s

CCPATH = /usr/bin
CC  = $(CCPATH)/$(HOST)-gcc
RC = $(CCPATH)/$(HOST)-windres

INCLUDE = -I../include
CPPFLAGS += $(INCLUDE)

CFLAGS += -fno-strict-aliasing -DNSISCALL="__attribute__((__stdcall__))"
LDFLAGS += -mwindows -Wl,--kill-at -static-libgcc -shared

UNICODE := true
ifeq ($(UNICODE),true)
CFLAGS += -DUNICODE -D_UNICODE
LDFLAGS += -municode
endif

DLLNAME = Printer.dll
DLL_OBJS = pluginapi.o printer.o

RES_SRC  = printer.rc
RES_OBJ = printer-rc.o
LDLIBS = -lwinspool

all: $(DLLNAME)

$(DLLNAME): $(DLL_OBJS) $(RES_OBJ)
	$(CC) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(RES_OBJ): $(RES_SRC)
	$(RC) $(CPPFLAGS) -o $@ $<

clean:
	-rm $(DLLNAME) *.o

.PHONY: all clean
