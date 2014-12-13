CCPATH = /usr/bin
CC  = $(CCPATH)/i586-pc-mingw32-gcc
CXX = $(CCPATH)/i586-pc-mingw32-g++
CFLAGS = -march=i386 -g3 -O0 -Wall -s
DLLWRAP = $(CCPATH)/i586-pc-mingw32-dllwrap
DLLTOOL = $(CCPATH)/i586-pc-mingw32-dlltool
WINDRES = $(CCPATH)/i586-pc-mingw32-windres

DLL_NAME = printers.dll
DLL_OBJS = printers.o
DLL_EXP_DEF = printers.def
DLL_EXP_LIB = libprinters.a
DLL_LDLIBS = -lwinspool
DLL_LDFLAGS = -mwindows -s

RES_OBJS = printdlg.o

TESTWRAP = testwrap.exe
TESTWRAP_OBJS = testwrap.o

DLLWRAP_FLAGS = --driver-name $(CC) --def $(DLL_EXP_DEF)

all: $(DLL_NAME) $(TESTWRAP)

$(DLL_NAME): $(DLL_OBJS) $(RES_OBJS) $(DLL_EXP_DEF)
	$(DLLWRAP) $(DLLWRAP_FLAGS) -o $(DLL_NAME) \
	$(DLL_OBJS) $(RES_OBJS) $(DLL_LDFLAGS) $(DLL_LDLIBS)

$(DLL_EXP_LIB): $(DLL_EXP_DEF)
	$(DLLTOOL) --dllname $(DLL_NAME) --def $(DLL_EXP_DEF) \
	--output-lib $(DLL_EXP_LIB)

$(RES_OBJS): printdlg.rc
	$(WINDRES) -o $@ $<

$(DLL_EXP_DEF): $(DLL_OBJS)
	$(DLLTOOL) --export-all --output-def $@ $(DLL_OBJS)

$(TESTWRAP): $(TESTWRAP_OBJS)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	-rm *.o $(DLL_NAME) *.def *.exe

.PHONY: clean
