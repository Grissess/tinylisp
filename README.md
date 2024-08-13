TinyLISP
========

Overview
--------

*Tiny*LISP is a very small implementation of LISP which was originally based on
the "eval/apply" loop in the superb *Structure and Interpretation of Computer
Programs* (Abelson, Sussman, Sussman). Per the name, the codebase of TinyLISP
is quite minimal (e.g., the only built-in logic operator is NAND), currently
under 1.5kloc. (The previous ceiling of 1kloc was breached by coroutine
support.) It also has very little external dependencies, for portability; the
entire external interface (as defined in the `tl_interp` structure) consists
only of a function to read a character, put a character back to be read again,
and an output function similar to libc's `printf`.

The implementation is written in standards compliant C (ideally ANSI C,
although some LLVM- and GNU-specific areas are in architecture-specific areas),
and should be portable to other compilers that adhere to these standards. The
basic executable should be usable wherever a POSIX-compliant libc (threading
not required) is available. Special configurations may introduce special
compilation requirements; see especially [Small Systems](#Small_Systems) below.

Building
--------

`make help` in the repository root prints out a plethora of build information.
It's more up-to-date than this README; read it carefully, and type `make` when
you're ready.

Running
-------

`make run` in the repository root. Internally, this rule runs:

	cat std.tl - | $(DEBUGGER) ./tl

...which loads the `std.tl` "standard" library to provide access to more common
LISP functionality. It is worth noting that the majority of TinyLISP's language
is implemented in itself, using its powerful metaprogramming capabilities;
refer to the language specification in the documentation.

Many variations are possible; run `make help` to get a full list of influential
variables and their documentation.

As of late, on UNIX, the above can be emulated as

	$(DEBUGGER) ./tl std.tl

but the former syntax works as well.

Embedding
---------

Refer to `main.c` for a typical way to pass control to a running interpreter.
In general, the major interface will be the pair `tl_eval_and_then` to
initialize the evaluation of an expression, followed by `tl_run_until_done` to
crank the interpreter until the final continuation is called.

On ELF systems, TL supports `INITSCRIPTS`, embedded binary data that contains
programs that are "read from input" initially. This may be useful to set up a
standard environment in embedded applications. See the [Makefile](Makefile) for
further details.

Modules
-------

TinyLISP can be augmented with loadable modules, provided a suitable dynamic
linker interface exists. The default on Unix-like systems is to assume `dlopen`
and `dlsym` are functional. See [main.c](main.c) for the responsibilities of
the loading function.

Modules, in [mod](mod/), are not counted toward TinyLISP's line count. They do,
however, show how one could implement additional functionality, such as more
general transput and memory-handling capabilities. More libraries are subject
to be added at any time.

Small Systems
-------------

TinyLISP has an embedded "minilibc", which contains exactly enough libc to
allow TL to run, based on five interface functions defined in
[arch.h](minilibc/arch.h): file operations (read, write, flush), memory
operations (get heap, release heap), and halt. This interface is designed to be
intentionally simple to implement--in particular, heaps can be large and are
suballocated, and no assumptions are made about file descriptors other than 0,
1, or 2 (standard in, out, and error).

To use it, pass `USE_MINILIBC=1` to a `make` invocation, optionally passing
`MINILIBC_ARCH=` one of the choices below.

- `linsys`: Uses Linux x64 system calls directly. Aside from testing purposes,
  the resulting binary has no dynamic dependencies--it is "statically linked".

- `wasm`: Compiles to WASM. This feature usually requires `CC=clang`. The
  resulting module has [a few extra symbols](minilibc/arch/wasm.c) intended to
  help embedding. Additionally, no bit packing is done. All the interface
  functions are imported, but `fgetc` is generally advised against on JS
  platforms that don't support blocking (such as the Web APIs), so
  `tl_apply_next` should be used directly instead. An example implementation is
  in [the `wasm` directory](wasm/), which can be used if this repository is
  served from a standard HTTP server.

- `rv32im`: A RISC-V RV32IM image is built. This is designed to be hosted in
  the RISC-V SoC in
  [Logisim-evolution](https://github.com/logisim-evolution/logisim-evolution).
  A basic memory map is:
	- 0x000_000 - 0x100_000: ROM, text sections and rodata loaded by the ELF;
	- 0x100_000 - 0x300_000: a "JTAG"-like interface
	- 0x300_000 - (0x300_000 + MEM_SIZE (default 0x100_000)): RAM
  These are the defaults, but can be overridden by adding to `DEFINES`. See
  [rv32im.c](minilibc/arch/rv32im.c) for details.

Other implementations are welcome to be added!

`minilibc` is not designed to be performant nor standards-compliant, and is not
counted toward TinyLISP's own line counts. However, it is implemented in much
the same brevity as TinyLISP, and thus can serve as a didactic example of libc
internals. In particular, it defines many `printf` variants and a functional
`malloc` based on the K&R malloc, but with adjacent region merging.

Language
--------

`tl_read` in `read.c` is the lexical analyzer. In short, a lexeme may be:

- `"` (double quote) bounds a symbol, which may contain any character other than `"`;
- `()` (parentheses) bound a proper list of lexemes;
  - ... except that a `. DATUM )` at the end of a list creates an improper
  	list; in particular `(A . B)` is a standard `cons` pair. Nowhere else is
  	`.` special, and it is treated as a symbol in those places.
- all digits introduce decimal numbers;
- whitespace terminates lexemes, including symbols, and is skipped;
- `;` and everything following it until newline is ignored (as comments);
- every other character represents itself within a symbol.

There is one caveat: at runtime, a TinyLISP program may introduce "prefices"
using `tl-prefix`, which translates into a proper list with the head being the
specified symbol or expression, and the second item being the entire prefixed
expression. This mechanism is used to implement `'` as `quote`, for example. In
the current iteration, prefices may only be one character.

Evaluation
----------

Evaluation rules are as follows:

- Numbers, as well as all forms of cfunction or LISP function, evaluate to themselves ("are self-valuating");
- Symbols evaluate to their "bound" value in the current environment (see below);
- Lists represent function application, with the head assumed to be callable, and the tail as arguments.

For example, the expression:

	(+ 3 5)

Evaluates:

1. The application, which pushes a continuation as it must evaluate:
	1. `+`, a symbol, which evaluates to the binding to `tl_cfbv_add`, a cfunction *by-value* callable, which invokes immediate evaluation of the arguments:
		1. `3`, which self-evaluates to 3,
		2. `5`, which self-evaluates to 5,
2. ...once the callable (and all arguments) have evaluated, the function (`tl_cfbv_add`) is applied with the argument list (`(3 5)`), producing the value 8.

As another carefully selected example, consider:

	(define foo #t)

...which evaluates:

1. The application, which pushes a continuation as it evaluates:
	1. `define`, which evaluates to the binding of `tl_cf_define`, a *non-by-value* cfunction--therefore, the *syntax* of the arguments are captured,
2. the function itself (`tl_cf_define`) is applied to the arguments `(foo #t)`, within which the second argument is pushed for evaluation:
	1. `#t` evaluates to `tl-#t` within the interpreter;
3. ...and the continuation for `tl_cf_define` binds this value `#tl-t` to the symbol `foo` within the current environment, and evaluates to `tl-#t`.

These examples demonstrate that TL has two modes of evaluation: strict
(call-by-value, or *direct*) evaluation, as with `+` above, and non-strict
(call-by-name, or *syntactic*) evaluation, as with `define`. Additionally, TL
can mix them freely; the result of `define` can be evaluated eagerly within
another function, or be deferred within another syntactic invocation. This
dichotomy is available to language users, too, as the difference between
`lambda` and `macro`, respectively. It is the power of this dichotomy that
allows LISP to effortlessly intermingle data and code; for a complete example,
see the definition of `quasiquote` in `std.tl`.

`eval` is the arrow between syntax and value. The internal implementation is
`tl-eval-in&`, or `tl_eval_and_then`, which captures a syntax and evaluates it
to a value (eventually--so long as the continuation is actually reached, for
example), which it then passes to its continuation. Note that TL must maintain
an in-code distinction between syntactic and direct values; for example:

	(tl-eval-in& (tl-env) 2 display)

...will display 2 (recall that numbers are self-valuating). However:

	(tl-eval-in& (tl-env) 2 define)

...will attempt to call `define` with the arguments `(2)`, where it is
understood that 2 is a direct (non-syntactic) value. Because `define` expects a
syntax to capture, the interpreter throws an error and abandons the evaluation.

The general rule for language users is as follows: if you expect a real value,
use `lambda`; if you expect syntax, use `macro`. For example, `2`, `(begin 2)`,
`((lambda () 2))`, and `(+ 1 1)` are several different syntaxes that all result
in the same direct value (`2`). In the mixed case where some arguments need to
be syntactic (using `define` as a prototypal example), use a `macro` to capture
syntax, and selectively use `tl-eval-in&` or variants to convert some syntax to
values.

TL macros are more general than some other implementations of "macros" that are
only allowed to rewrite syntax. A common idiom for implementing those macros in
TL is:

```
(define example (macro (arg) env (tl-eval-in env `( ... ))))
```

where `...` is the syntactic translation, using the appropriate `quasiquote`
macros. But, of course, macros are not so restricted--they can implement
arbitrary functionality, including invoking other macros. This can be used for
advanced features, such as lazy evaluation.

In short, the TL evaluator is a "Call by Push Value" (CBPV) interpreter, and
can thus implement many different modalities of lambda calculus evaluation.

License
-------

TinyLISP is distributed under the terms of the Apache 2.0 License. See
`COPYING` for more information.

Support
-------

Please feel free to leave issues on this repository, or submit pull requests. I
will make every effort to respond to them in a timely fashion.

Contributing
------------

Contributions are welcome--the most wieldy being pull requests, but I can
negotiate other methods of code delivery. I retain the right to curate the code
in this repository, but you are within your licensed right to make forks for
your own purposes. Nevertheless, contributing back upstream is welcome!

Here are some easy things to look at:

- Documentation improvements and clean-up;
- Code cleanup (especially anything that makes it brief without being inscrutable);
- Finding bugs (they are inevitable, after all);
- Improving workflows (tell me what's hard to accomplish).
