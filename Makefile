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
INTERPRETER ?= tl

define newline


endef
define tab
	
endef

define help_text
Type 'make' (or make -jN, for N parallel jobs) to make TinyLISP.

Targets (pass ONE of these to make):
	all: The TinyLISP interpreter (or output library).
	$(INTERPRETER): the current output file.
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
		Items that will be appended to CFLAGS. See
		'Preprocessor configuration', below.

	LD = $(LD)
		Your linker.

	LDFLAGS = $(LDFLAGS)
		Additional options for your linker.

	PLAT = $(PLAT)
		Your platform:
			UNIX: Something that resembles POSIX.
			WINDOWS: Something that resembles Windows.
			WASM: A WASM-bytecode runtime.
 	
	USE_MINILIBC = $(USE_MINILIBC)
		If nonempty, use Minilibc (a minimal C library)
		and build tl statically. (This also affects
		modules, see below.)

	MINILIBC_ARCH = $(MINILIBC_ARCH)
		The architecture backend used by minilibc to
		interact with the world. Valid values are {
		$(basename $(notdir $(wildcard minilibc/arch/*.c)))
		}; the default is 'linsys'.

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

Preprocessor configuration:
	While these can be passed in CFLAGS, the variable
	DEFINES is provided for your convenience to add these
	flags. It defaults to empty, while CFLAGS may be changed
	by your target or other configuration. Most of these are
	optional, but some may be forced by CFLAGS on various
	targets.

- Debugging macros:
	The debug macros will generally make your executable
	larger and very much slower--use them sparingly.

	-DNS_DEBUG
		Namespace debugging; prints out large amounts of
		debugging information about the symbol-interning
		"namespace" system.

	-DCONT_DEBUG
		Continuation debugging; prints out large amounts of
		information about the evaluation loop, including all
		the continuations encountered and the interpreter
		state at each tl_apply_next step.

	-DMEM_DEBUG
		Memory debugging; if USE_MINILIBC=1, this prints out
		large amounts of information about the Minilibc
		memory allocator, including every call to malloc()
		and free().

	-DMEM_INSPECT
		Memory inspection; if USE_MINILIBC=1, this adds the meminfo
		routine, which returns some information about the memory
		allocator (which adds a little overhead to each allocation),
		and the tl-meminfo builtin for viewing it from the REPL.

	-DGC_DEBUG
		Garbage collector debugging; prints out large
		amounts of information about every collector run,
		including every object encountered and freed.

	-DFAKE_ASYNC
		Move the handling of TL_RESULT_GETCHAR into main(),
		using the libc getchar() function, to more
		faithfully emulate asynchronous input targets.

	-DLOAD_DEBUG
		Function load debugging; prints out when the TL_INIT_ENTS are
		traversed, which usually contain built-in functions. This normally
		happens once during interpreter init, but also happens during module
		load, and can be useful to see which modules affect the environment.

- Platform macros:
	These macros usually express configuration options that
	may not be available on all platforms. Some platforms will force options
	from this list.

	-DPTR_LSB_AVAILABLE=X
		The bottom X bits of a pointer are available. If
		this is too few, it implies some other macros below,
		including NO_GC_PACK and NO_MEM_PACK.

	-DNO_GC_PACK
		Don't bitpack the GC marks into the intrusive list;
		this makes objects larger, but is necessary on some
		targets which either can't align allocations or have
		native alignments of single bytes. Implied if
		PTR_LSB_AVAILABLE < 2.

	-DNO_MEM_PACK
		Don't bitpack the malloc mark into the intrusive
		list; this means allocations will have more overhead
		per-allocation, but is strictly necessary on some
		targets as above. Implied if PTS_LSB_AVAILABLE == 0;
		MEM_DEBUG causes a similar change, but continues to
		pack the bit anyway for debugging purposes unless
		this is turned off.
	
	-DHAVE_SYSTEMTAP
		Compile against the SystemTap userspace libraries to
		instrument the binary with UDSTs.

	-DTL_DEFAULT_GC_EVENTS=X
		After X events, attempt an automatic GC. A GC
		already occurs as part of the main REPL, but this
		allows collections to occur in the middle of user
		code. This will be deprecated once a load-based GC
		strategy is implemented. The default is 65536.

	Your current CFLAGS, which include DEFINES, are:
		$(CFLAGS)
endef
export help_text

ALL_MODULES = io mem
MODULES ?= all
MODULES_BUILTIN ?=
CFLAGS += -D$(PLAT)

polyfill = -Iminilibc/polyfill/$(1)

ifeq ($(MODULES),all)
	MODULES := $(ALL_MODULES)
endif

ifneq ($(USE_MINILIBC),)
	CFLAGS += -Iminilibc -nostdlib -static
	OBJ += $(patsubst %,minilibc/%.o,string stdio assert stdlib ctype unistd errno)
	MINILIBC_ARCH ?= linsys
	MINILIBC_MK := minilibc/arch/$(MINILIBC_ARCH).mk
ifeq ($(V),0)
	# Nothing at all
else ifeq ($(V),1)
$(info $(tab)INCL$(tab)$(MINILIBC_MK))
else
$(info $(tab)-include $(MINILIBC_MK))
endif
-include $(MINILIBC_MK)
	MINILIBC_OBJ := minilibc/arch/$(MINILIBC_ARCH).o
	OBJ += $(MINILIBC_OBJ)
$(MINILIBC_OBJ): CFLAGS += -I.
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

all: $(INTERPRETER) $(LIB)

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
	@echo "$$help_text"

cmd_run = cat std.tl - | $(DEBUGGER) ./$(INTERPRETER)
quiet_run = RUN
run: $(INTERPRETER)
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

cmd_clean = rm $(OBJ) $(INTERPRETER)
quiet_client = CLEAN
clean:
	$(call cmd,clean)

cmd_install = install -D -t $(DESTDIR)/bin/ $^
quiet_install = INSTALL\t$(DESTDIR)/bin/$^
install: tl
	$(call cmd,install)

cmd_tl = $(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
quiet_tl = LD\t$@
$(INTERPRETER): $(OBJ)
	$(call cmd,tl)

cmd_so = $(CC) -shared -o $@ $(CFLAGS) $< $(LDFLAGS)
quiet_so = CC(SO)\t$@
%.so: %.o
	$(call cmd,so)

cmd_o = $(CC) $(CFLAGS) -o "$@" -c "$<"
quiet_o = CC\t$@
%.o: %.c
	$(call cmd,o)
%.o: %.s
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
