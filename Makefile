LIBOBJ := builtin.o env.o eval.o interp.o object.o print.o read.o debug.o ns.o
APPOBJ ?= main.o
OBJ := $(LIBOBJ) $(APPOBJ)
LIB := 
INITSCRIPT_OBJ :=
SRC = $(patsubst %.o,%.c,$(OBJ))
CFLAGS ?= -g -std=gnu99 -DDEBUG $(ADD_CFLAGS)
LDFLAGS ?= $(ADD_LDFLAGS)
DESTDIR ?= /usr/local/
BINPATH ?= $(DESTDIR)/bin/
LIBPATH ?= $(DESTDIR)/lib/
INCPATH ?= $(DESTDIR)/include/
DATAPATH ?= $(DESTDIR)/share/
CC ?= gcc
AR ?= ar
PLAT ?= UNIX  # TODO: figure this out somehow
V ?= 1
INTERPRETER ?= tl
LIBRARY ?= libtl

define newline


endef
define tab
	
endef
comma := ,

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
		Options for your C compiler. This contains the actual set used
		during compilation; as such, it can change based on platforms
		and defaults defined in this Makefile. `make help` will, when
		passed other configuration, show you what would have been used.
		Setting this from the command line has the effect of overriding
		these defaults in most circumstances, which may be undesirable.

	ADD_CFLAGS = $(ADD_CFLAGS)
		Items that will be appended to CFLAGS. This is left undefined
		by default, and thus safe to set from the command line. Most
		commonly used for introducing preprocessor definitions; for
		that purpose, see 'Preprocessor configuration', below.

	LD = $(LD)
		Your linker.

	LDFLAGS = $(LDFLAGS)
		Options for your linker. The same caveats as CFLAGS apply.

	ADD_LDFLAGS = $(ADD_LDFLAGS)
		Items appended to LDFLAGS. This is left undefined by default,
		and safe to set from the command line.

	AR = $(AR)
		Your archiver, for $$STATIC builds.

	STATIC = $(STATIC)
		When non-empty, a static library is requested. By default, a shared
		library is built.
	
	INTERPRETER = $(INTERPRETER)
		The name of the interpreter executable to be built.

	APPOBJ = $(APPOBJ)
		The object files linked into the interpreter. Setting this to another
		set of files can build custom, bespoke interpreters.

	LIBRARY = $(LIBRARY)
		When non-empty, the library prefix that will be built. It should
		probably have 'lib' at the beginning to be found by the usual '-l'
		argument. A suffix appropriate to $$STATIC will be appended (usually
		.a, .so, or .dll). When empty, no library is built at all. This is
		forced to be empty by USE_MINILIBC, as those targets categorically
		expect static linkage.

	PLAT = $(PLAT)
		Your platform:
			UNIX: Something that resembles POSIX.
			WINDOWS: Something that resembles Windows.
			WASM: A WASM-bytecode runtime.
 	
	USE_MINILIBC = $(USE_MINILIBC)
		If nonempty, use Minilibc (a minimal C library)
		and build tl statically. (This also affects
		modules and linkage, see above and below.)

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
		exactly the specified order, before command line arguments.

	V = $(V)
		Build verbosity:
			0: Output nothing except compiler errors.
			1: Output a simplified line (and the above).
			2: Show the full command line, as usual.
			3: Show everything, including this infrastructure.

	DESTDIR = $(DESTDIR)
		The install root. This is the prefix for many other install
		directories.

	BINPATH = $(BINPATH)
		The path to installed binaries. This should be in your shell $$PATH.

	LIBPATH = $(LIBPATH)
		The path to libraries. For non-$$STATIC builds, this should be in your
		linker's search path. Modules are installed here, too. (Note that TL
		itself does not yet have a path search facility.

	INCPATH = $(INCPATH)
		THe path to header files. This should be in your compiler's default
		search path.

	DATAPATH = $(DATAPATH)
		The path for platform-inspecific data. The default init script is
		copied under here.

Preprocessor configuration:
	While these can be passed in CFLAGS, the variable
	ADD_CFLAGS is provided for your convenience to add these
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
	-DGC_DEBUG=1
		Garbage collector debugging; prints out large amounts of
		information about every collector run, including every object
		encountered and freed when defined to at least 1.

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

	Your current CFLAGS, which include ADD_CFLAGS, are:
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
	LIBRARY :=
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
	APPOBJ += $(INITSCRIPT_OBJ)
$(APPOBJ): CFLAGS += -DINITSCRIPTS="$(INITSCRIPTS)"
endif

ifeq ($(STATIC),)
	CFLAGS += -fPIC
	LIBSUFF := .so
	LDFLAGS += -Wl,-rpath='$$ORIGIN'
$(APPOBJ): CFLAGS += -DSHARED_LIB
else
	LIBSUFF := .a
$(APPOBJ): CFLAGS += -DSTATIC_LIB
$(INTERPRETER): LDFLAGS += -static
endif

ifeq ($(LIBRARY),)
	LIBTARGET :=
	APPDEP := $(OBJ)
else
	LIBTARGET := $(LIBRARY)$(LIBSUFF)
	APPDEP := $(APPOBJ) $(LIBTARGET)
$(INTERPRETER): LDFLAGS += -L.
endif

intolink = $(patsubst lib%.so,-l%,$(patsubst lib%.a,-Wl$(comma)--whole-archive -l% -Wl$(comma)--no-whole-archive,$(1)))

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

all: $(LIBTARGET) $(INTERPRETER) $(LIB)

define showconfig_text
	OBJ = $(OBJ)
	LIBTARGET = $(LIBTARGET)
	LIB = $(LIB)
	PLATFORM = $(PLATFORM)
	MODULES = $(MODULES)
	MODULES_BUILTIN = $(MODULES_BUILTIN)
	INITSCRIPTS = $(INITSCRIPTS)

	CC = $(CC)
	CFLAGS = $(CFLAGS)
	LD = $(LD)
	LDFLAGS = $(LDFLAGS)
endef
export showconfig_text

define cmd_showconfig
	echo "$$showconfig_text"
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

cmd_clean = rm $(OBJ) $(INTERPRETER) $(LIBRARY).a $(LIBRARY).so || true
quiet_client = CLEAN
clean:
	$(call cmd,clean)

define cmd_install
	install -D -t "$(BINPATH)" "$(INTERPRETER)"
	$(Q)install -D -t "$(LIBPATH)" "$(LIBTARGET)"
	$(Q)install -D -t "$(INCPATH)" "tinylisp.h"
	$(Q)install -D -t "$(LIBPATH)/tl/mod/" $(MODULE_OBJECTS)
	$(Q)install -D -t "$(DATAPATH)/tl/" std.tl
endef
quiet_install = INSTALL
install: $(INTERPRETER) $(LIBTARGET) $(MODULE_OBJECTS) std.tl tinylisp.h
	$(call cmd,install)

cmd_tl = $(CC) $(CFLAGS) $(call intolink,$^) $(LDFLAGS) -o $@
quiet_tl = LD\t$@
$(INTERPRETER): $(APPDEP)
	$(call cmd,tl)

cmd_so = $(CC) -shared -o $@ $(CFLAGS) $^ $(LDFLAGS)
quiet_so = CC(SO)\t$@
%.so: %.o
	$(call cmd,so)
$(LIBRARY).so: $(LIBOBJ)
	$(call cmd,so)

cmd_o = $(CC) $(CFLAGS) -o "$@" -c "$<"
quiet_o = CC\t$@
%.o: %.c
	$(call cmd,o)
%.o: %.s
	$(call cmd,o)

cmd_a = $(AR) rcs "$@" $^
quiet_a = AR\t$@
$(LIBRARY).a: $(LIBOBJ)
	$(call cmd,a)

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
