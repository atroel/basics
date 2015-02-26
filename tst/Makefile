.PHONY: all clean
.NOTPARALLEL: clean

export CFLAGS=-O0 -ggdb

all clean:
	+@$(MAKE) -C ../src GOALS=libb6.a R=$(CURDIR)/lib $@
	+@$(MAKE) -f ../build/Makefile LDFLAGS="$(CURDIR)/lib/libb6.a -lpthread" $@
