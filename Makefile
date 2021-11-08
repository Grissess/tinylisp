OBJ := builtin.o env.o eval.o interp.o object.o print.o read.o debug.o ns.o main.o
LIB := 
INITSCRIPT_OBJ :=
SRC = $(patsubst %.o,%.c,$(OBJ))
CFLAGS ?= -g -std=gnu99 -DDEBUG $(DEFINES)
LDFLAGS ?= 
DESTDIR ?= /usr/local/
CC ?= gcc
PLAT ?= UNIX  # TODO: figure this out somehow
V ?= 1

define newline


endef

define help_text
Type 'make' (or make -jN, for N parallel jobs) to make TinyLISP.

Targets (pass ONE of these to make):
	tl: the TinyLISP executable.
	all: equivalent to tl.
	dist: the tinylisp.tar.xz source tarball.
	clean: remove tl and all build intermediates.
	install: install tl to DESTDIR
		(currently, DESTDIR = $(DESTDIR))
	ns_test: the namespace test program.
	help: this message.
	showconfig: show important variables (for debugging).

Influential variables (and their current values):
	CC = $(CC)
		Your C compiler.

	CFLAGS = $(CFLAGS)
		Additional options for your C compiler.

	DEFINES = $(DEFINES)
		Items that will be appended to CFLAGS.

	LD = $(LD)
		Your linker.

	LDFLAGS = $(LDFLAGS)
		Additional options for your linker.

	PLAT = $(PLAT)
		Your platform:
			UNIX: Something that resembles POSIX.
			WINDOWS: Something that resembles Windows.
 	
	USE_MINILIBC = $(USE_MINILIBC)
		If nonempty, use Minilibc (a minimal C library)
		and build tl statically. (This also affects
		modules, see below.)

	MODULES = $(MODULES)
		Modules to build. These will be built as shared
		objects (libraries) unless USE_MINILIBC is set,
		in which case they will be statically linked
		and run at program start.

		If the value is "all", all modules this Makefile
		knows about will be built:
			ALL_MODULES = $(ALL_MODULES)

	INITSCRIPTS = $(INITSCRIPTS)
		Initscripts that will be run for tl (specifically,
		not other embedders) before any user input is
		processed. Scripts will be embedded and run in
		exactly the specified order.

	V = $(V)
		Build verbosity:
			0: Output nothing except compiler errors.
			1: Output a simplified line (and the above).
			2: Show the full command line, as usual.
			3: Show everything, including this infrastructure.
endef

ALL_MODULES = io
MODULES ?= all
MODULES_BUILTIN ?=
CFLAGS += -D$(PLAT)

ifeq ($(MODULES),all)
	MODULES := $(ALL_MODULES)
endif

ifneq ($(USE_MINILIBC),)
	CFLAGS += -Iminilibc -nostdlib -nostartfiles -static
	OBJ += $(patsubst %,minilibc/%.o,string stdio assert stdlib ctype unistd)
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

ifneq ($(INITSCRIPTS),)
	INITSCRIPT_OBJ += $(addsuffix .o,$(INITSCRIPTS))
	OBJ += $(INITSCRIPT_OBJ)
	CFLAGS += -DINITSCRIPTS="$(INITSCRIPTS)"
endif

Q := @
quiet := cmd_
cmd = @printf '\t$($(quiet)$(1))\n'; $(cmd_$(1))
ifeq ($(V),0)
	cmd = @$(cmd_$(1))
endif
ifeq ($(V),1)
	quiet := quiet_
endif
ifeq ($(V),3)
	Q :=
	cmd = printf 'rule $(1): $(cmd_$(1))'; $(cmd_$(1))
endif

.PHONY: all clean run dist docs showconfig help
.PRECIOUS: $(INITSCRIPTS)
.SUFFIXES:

all: tl $(LIB)

define cmd_showconfig
	echo "OBJ = $(OBJ)"
	$(Q)echo "LIB = $(LIB)"
	$(Q)echo "MODULES = $(MODULES)"
	$(Q)echo "MODULES_BUILTIN = $(MODULES_BUILTIN)"
	$(Q)echo "INITSCRIPTS = $(INITSCRIPTS)"
endef
quiet_showconfig = SHOWCONFIG
showconfig:
	$(call cmd,showconfig)

help:
	@printf "$(subst $(newline),\n,$(help_text))\n"

cmd_run = cat std.tl - | $(DEBUGGER) ./tl
quiet_run = RUN
run: tl 
	$(call cmd,run)

dist: tinylisp.tar.xz

define cmd_docs
	doxygen Doxyfile
	$(Q)sphinx-build -b html . ./doc_html
endef
quiet_docs = DOCS
docs: Doxyfile
	$(call cmd,docs)

define cmd_tinylisp_tar_xz
	rm $@ || true
	$(Q)xz -ze $<
endef
quiet_tinylisp_tar_xz = XZ\t$@
tinylisp.tar.xz: tinylisp.tar
	$(call cmd,tinylisp_tar_xz)

define cmd_tinylisp_tar
	rm $@ || true
	$(Q)tar cvf $@ $^
endef
quiet_tinylisp_tar = TAR\t$@
tinylisp.tar: $(SRC) std.tl test.tl Makefile
	$(call cmd,tinylisp_tar)

cmd_clean = rm $(OBJ) tl
quiet_client = CLEAN
clean:
	$(call cmd,clean)

cmd_install = install -D -t $(DESTDIR)/bin/ $^
quiet_install = INSTALL\t$(DESTDIR)/bin/$^
install: tl
	$(call cmd,install)

cmd_tl = $(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
quiet_tl = LD\t$@
tl: $(OBJ)
	$(call cmd,tl)

cmd_so = $(CC) -shared -o $@ $(CFLAGS) $< $(LDFLAGS)
quiet_so = CC(SO)\t$@
%.so: %.o
	$(call cmd,so)

cmd_o = $(CC) $(CFLAGS) -o "$@" -c "$<"
quiet_o = CC\t$@
%.o: %.c
	$(call cmd,o)

define cmd_initscript
	echo "TARGET(binary) OUTPUT_FORMAT($$(ld --print-output-format)) SECTIONS { tl_init_scripts : { *(.data) }}" | ld -T /dev/stdin -r -o "$@" "$<"
endef
quiet_initscript = INISCR\t$@
$(INITSCRIPT_OBJ): %.o: %
	$(call cmd,initscript)

$(OBJ) $(LIB): tinylisp.h

cmd_ns_test = $(CC) -DNS_DEBUG $(CFLAGS) $^ -DNS_TEST $(LDFLAGS) -o $@
quiet_ns_test = LD\t$@
ns_test: ns.c interp.c object.c builtin.c print.c env.c eval.c read.c
	$(call cmd,ns_test)
