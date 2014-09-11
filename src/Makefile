export EXTRA_CPPFLAGS=-I../include
export EXTRA_CFLAGS+=-g0 -O3 -DNDEBUG
export EXTRA_CFLAGS+=-fomit-frame-pointer -ffast-math -msse -mfpmath=sse

export SRC=$(wildcard *.c)
export A=libb6.a
export L=libb6.so.1

.PHONY: all clean mrproper

all clean:
	@$(MAKE) -f ../build/Makefile $@
