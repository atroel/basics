CC=$(CROSS_COMPILE)gcc -fmessage-length=78
AR=$(CROSS_COMPILE)ar

CPPFLAGS=-MMD -MF "$(@:%.o=%.d)" -MT "$@ $(<:%.c=%.d)" $(EXTRA_CPPFLAGS)
CFLAGS=$(CPPFLAGS) -Wall $(EXTRA_CFLAGS)
LDFLAGS=$(EXTRA_LDFLAGS)

CFLAGS_SO=-fPIC $(CFLAGS)
LDFLAGS_SO=-shared -Wl,-soname,"$@" $(LDFLAGS)

ifndef VERBOSE
do = @(echo " $(1)"; $(2))
else
do = $(2)
endif

OBJ+=$(SRC:.c=.o)
SOBJ+=$(SRC:.c=.so)
DEP=$(SRC:.c=.d)

.PHONY: clean all

all: $(X) $(A) $(L)

clean:
ifneq ($(strip "$(X)"),"")
	$(call do, "CLEAN $(X)", $(RM) $(X) $(OBJ) $(DEP))
endif
ifneq ($(strip "$(A)"),"")
	$(call do, "CLEAN $(A)", $(RM) $(A) $(OBJ) $(DEP))
endif
ifneq ($(strip "$(L)"),"")
	$(call do, "CLEAN $(L)", $(RM) $(L) $(SOBJ) $(DEP))
endif

$(X): $(OBJ)
	$(call do, "LINK  $@", $(CC) -o "$@" $^ $(LDFLAGS))

$(L): $(SOBJ)
	$(call do, "LINK  $@", $(CC)  -o "$@" $^ $(LDFLAGS_SO))

$(A): $(OBJ)
	$(call do, "ARCH  $@", $(AR) -rcs "$@" $^ $(LDFLAGS))

%.so: %.c
	$(call do, "COMP  $@", $(CC) $(CFLAGS_SO) -c "$<" -o "$@")

%.o: %.c
	$(call do, "COMP  $@", $(CC) $(CFLAGS) -c "$<" -o "$@")

-include $(DEP)
