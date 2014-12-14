CCPATH = /usr/bin
HOST=i686-w64-mingw32
CC  = $(CCPATH)/$(HOST)-gcc
CXX = $(CCPATH)/$(HOST)-g++
DLLWRAP = $(CCPATH)/$(HOST)-dllwrap
DLLTOOL = $(CCPATH)/$(HOST)-dlltool
WINDRES = $(CCPATH)/$(HOST)-windres

INCLUDE = -I../include
CFLAGS = $(INCLUDE) -march=i386 -g3 -O0 -Wall -s

DLL_NAME = printers.dll
DLL_OBJS = pluginapi.o printers.o
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
	$(WINDRES) $(INCLUDE) -o $@ $<

$(DLL_EXP_DEF): $(DLL_OBJS)
	$(DLLTOOL) --export-all --output-def $@ $(DLL_OBJS)

$(TESTWRAP): $(TESTWRAP_OBJS)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	-rm *.o $(DLL_NAME) *.def *.exe

.PHONY: clean
