OBJ := builtin.o env.o eval.o interp.o object.o print.o read.o debug.o main.o
SRC = $(patsubst %.o,%.c,$(OBJ))
CFLAGS ?= -g -std=gnu99 -DDEBUG $(DEFINES)
LDFLAGS ?= 
DESTDIR ?= /usr/local/
CC ?= gcc

ALL_MODULES = io
MODULES ?= all

ifeq ($(MODULES),all)
	MODULES := $(ALL_MODULES)
endif

ifneq ($(MODULES),)
	CFLAGS += -DCONFIG_MODULES="$(MODULES)" -I.
	LDFLAGS += -ldl

	MODULE_OBJECTS := $(patsubst %,mod/%.o,$(MODULES))
	OBJ += $(MODULE_OBJECTS)
endif

ifneq ($(USE_MINILIBC),)
	CFLAGS += -Iminilibc -nostdlib
	OBJ += $(patsubst %,minilibc/%.o,string stdio assert stdlib ctype)
	MINILIBC_ARCH ?= linsys
	OBJ += minilibc/arch/$(MINILIBC_ARCH).o
endif

.PHONY: all clean run dist docs

all: tl

run: tl 
	cat std.tl - | $(DEBUGGER) ./tl

dist: tinylisp.tar.xz

docs: Doxyfile
	doxygen Doxyfile
	sphinx-build -b html . ./doc

tinylisp.tar.xz: tinylisp.tar
	rm $@ || true
	xz -ze $<

tinylisp.tar: $(SRC) std.tl test.tl Makefile
	rm $@ || true
	tar cvf $@ $^

clean:
	rm $(OBJ) tl

install: tl
	install -D -t $(DESTDIR)/bin/ $?

tl: $(OBJ)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(OBJ): tinylisp.h
