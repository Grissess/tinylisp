OBJ := builtin.o env.o eval.o interp.o object.o print.o read.o debug.o main.o
LIB := 
SRC = $(patsubst %.o,%.c,$(OBJ))
CFLAGS ?= -g -std=gnu99 -DDEBUG $(DEFINES)
LDFLAGS ?= 
DESTDIR ?= /usr/local/
CC ?= gcc

ALL_MODULES = io
MODULES ?= all
MODULES_BUILTIN ?=

ifeq ($(MODULES),all)
	MODULES := $(ALL_MODULES)
endif

ifneq ($(USE_MINILIBC),)
	CFLAGS += -Iminilibc -nostdlib -nostartfiles -static
	OBJ += $(patsubst %,minilibc/%.o,string stdio assert stdlib ctype)
	MINILIBC_ARCH ?= linsys
	OBJ += minilibc/arch/$(MINILIBC_ARCH).o
	MODULES_BUILTIN := $(MODULES)
	MODULES :=
endif

ifneq ($(MODULES),)
	CFLAGS += -DCONFIG_MODULES="$(MODULES)"
	LDFLAGS += -ldl -rdynamic

	MODULE_OBJECTS := $(patsubst %,mod/%.so,$(MODULES))
$(MODULE_OBJECTS): CFLAGS += -I. -DMODULE
	LIB += $(MODULE_OBJECTS)
endif

ifneq ($(MODULES_BUILTIN),)
	CFLAGS += -DCONFIG_MODULES_BUILTIN="$(MODULES_BUILTIN)"
	MODULES_BUILTIN_OBJECTS := $(patsubst %,mod/%.o,$(MODULES_BUILTIN))
$(MODULES_BUILTIN_OBJECTS): CFLAGS += -I. -DMODULE -DMODULE_BUILTIN
	OBJ += $(MODULES_BUILTIN_OBJECTS)
endif

.PHONY: all clean run dist docs

all: tl $(LIB)

run: tl 
	cat std.tl - | $(DEBUGGER) ./tl

dist: tinylisp.tar.xz

docs: Doxyfile
	doxygen Doxyfile
	sphinx-build -b html . ./doc_html

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

%.so: %.o
	$(CC) -shared -o $@ $(CFLAGS) $^ $(LDFLAGS)

$(OBJ) $(LIB): tinylisp.h
