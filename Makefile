.PHONY: all clean mrproper doc

all:
	@$(MAKE) -C src $@
	@$(MAKE) -C tst $@

doc:
	@$(MAKE) -C $@

clean:
	@$(MAKE) -C src $@
	@$(MAKE) -C tst $@
	@$(MAKE) -C doc $@

mrproper: clean
	find . -name "*~" -exec rm {} \;
