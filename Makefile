.PHONY: all clean static src tst doc
.NOTPARALLEL: clean

all: src

clean:
	+@$(MAKE) -C src $@
	+@$(MAKE) -C tst $@
	+@$(MAKE) -C doc $@

static:
	+@$(MAKE) -C src GOALS=libb6.a

src tst doc:
	+@$(MAKE) -C $@
