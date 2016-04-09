export CFLAGS := -march=i686 -fomit-frame-pointer -ftracer -Os -Wall
export HOST := i686-w64-mingw32

export CC := $(HOST)-gcc
export RC := $(HOST)-windres

all: nsisplugin

nsisplugin:
	make -C nsisplugin

clean:
	make -C nsisplugin clean

.PHONY: all clean nsisplugin
