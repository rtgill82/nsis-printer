export HOST := i686-w64-mingw32
export CFLAGS := -march=i686 -fomit-frame-pointer -ftracer -Os -Wall
export CPPFLAGS :=
export LDFLAGS :=

export CC := $(HOST)-gcc
export CXX := $(HOST)-g++
export DLLWRAP := $(HOST)-dllwrap
export DLLTOOL := $(HOST)-dlltool
export WINDRES := $(HOST)-windres

all: nsisplugin

nsisplugin:
	make -C nsisplugin

testwrap:
	make -C testwrap

clean:
	make -C nsisplugin clean
	make -C testwrap clean

.PHONY: all clean nsisplugin testwrap
