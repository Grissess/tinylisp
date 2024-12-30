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

Scripts can thus be run as

	./tl script.tl

and multiple scripts can be run sequentially by providing them in order, as in

	./tl std.tl lib.tl script.tl

but note that the interpreter will expect more data from `stdin` after all
scripts run. If this is undesirable, the standard interpreter provides a
built-in `tl-exit` which, when applied to an int, exits the interpreter with
that "exit status". Meanings differ between platforms, but POSIX generally
holds that only the least significant 8 bits are meaningfully provided to a
waiting parent process; what is done with this is, again, platform dependent,
but a POSIX shell usually interprets a value of 0 as "success" or "true" and
any other value as "failure" or "false", as is useful in the interpretation of
conditional expressions.

On POSIX systems, when running the interpreter using a non-TTY (according to
`isatty`) as input, the interpreter automatically suppresses prompt characters
(`> `) and statements about the value read or any result of evaluation which is
merely "true" (`tl-#t`) in the REPL. (Most built-in functions that don't return
a semantically-meaningful value, such as `tl-define`, return `tl-#t`.) This is
intended to make interoperability with pipelines easier. If this behavior is
undesirable, the default interpreter has a built-in `tl-quiet` which accepts
the following integer values:

- `0`: (`QUIET_OFF`) The default when run from a TTY, this shows a prompt (`>
  `) before reading each expression, announces the expression as read (`Read:
  `), and prefixes the result of evaluation with `Value: `). It also prints
  `Done.` when end-of-file is encountered.
- 1: (`QUIET_NO_PROMPT`) This does not show the prompts (`> `, `Read:`,
  `Value:`, and `Done.`), but still displays the value read and the result of
  evaluation.
- 2: (`QUIET_NO_TRUE`) The default when run from a non-TTY, this suppresses
  printing the value read and printing the true value `tl-#t`. Other values are
  still printed.
- 3: (`QUIET_NO_VALUE`) This suppresses all printing of values.

`tl-display` continues to print out formatted values in all cases.

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
the loading function. Modules may also be statically-linked, as is the default
on small systems (see below). Statically linked modules are not automatically
loaded by the interpreter interface; they have to be discovered in the link
process. See, again, [main.c](main.c), which does this after initializing the
intepreter. In general, downstreams may call statically initialized modules in
any way, at any time.

Modules, in [mod](mod/), are not counted toward TinyLISP's line count. They do,
however, show how one could implement additional functionality, such as more
general transput and memory-handling capabilities. More libraries are subject
to be added at any time, and these are not necessarily subject to the same
scrutiny as TinyLISP's own code. Cautious downstreams may wish to set `MODULES`
in the build process to a space-separated list of vetted modules, or an empty
string.

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

	(call/cc (lambda (cc) cc))

... successfully binds a continuation object to `cc` in the scope of the
`lambda` user function. However:

	(call/cc (macro (cc) env cc))

... will attempt to call the user `macro` with a continuation as an argument;
as described below, continuations are values that do _not_ have a
self-valuating syntax, since they capture the delicate internal state of the
interpreter. Because `macro`s expect a syntax to capture, the interpreter
throws an error (`"invoke macro/cfunc with non-syntactic arg"`) and abandons
the evaluation.

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

Errors
------

TinyLISP is not a total language; it is possible for computations to not only
run indefinitely, but also to be abandoned. While there are many ways to do
this (including invoking a continuation, as described below), likely the most
mundane is the generation of an `error`. Various built-in functions will raise
errors if they are inappropriately applied (to the wrong types of arguments, to
invalid numbers of arguments, or--as above--to direct values when they are
expecting syntax, and so forth). User code can invoke the built-in function
`tl-error` to raise an error directly.

TinyLISP has a so-called "rescue stack", which is initially empty. When it is
empty, an error will propagate directly to the C top-level, wherein
`tl_apply_next` returns `TL_RESULT_DONE` (which ends any running
`tl_run_until_done` call), and the interpreter's `error` field will be
non-`NULL`. (This means the empty list, which is incidentally represented as
`NULL`, is never a valid error value.) An interpreter in error state will not
resume until `error` is cleared; usually, the entire computation is abandoned
with `tl_interp_reset` before beginning a new evaluation. However, it is
possible to restart the same computation, on the understanding that the values
may not be sensible due to the error condition; most built-in functions will
return `tl-#f` (the standard "false" symbol), which may cause further errors.

User code can use `tl-rescue`, which expects a callable argument that takes no
parameters, to push a continuation onto the rescue stack. When this stack isn't
empty and an error is generated, the `error` object is passed to the topmost
continuation--in effect, the program behaves as if `tl-rescue` returned the
error object. If no error is generated, `tl-rescue` evaluates to the result of
evaluating the callable. Note that there is no guaranteed way to distinguish
successful results and `error` objects without construction: it may be
necessary to structure the evaluation of the callable to guarantee that a
returned value is unlikely to be mistaken for an error.

C programs may push continuations (or a `TL_THEN` C continuation) onto the
rescue stack as well, provided it is careful to balance with an appropriate
`TL_DROP_RESCUE` special pushed onto the continuation stack simultaneously.
This permits C code to react to errors without having them propagate to the
top-level `tl_run_until_done` loop if so desired. See the comments in `eval.c`
and relevant definitions in `tinylisp.h` for more details.

While this mechanism can be used for non-local stack-based return like C's
`setjmp`/`longjmp`, it is internally built on continuations, which are a far
more powerful non-local control flow mechanism; as such, most TL users are
expected to use them directly instead; see Continuations, below. Nevertheless,
errors are the correct choice to represent divergent computations, usually due
to incorrectness.

General Parameter Syntax
------------------------

User functions (both `lambda` and `macro`) are defined with a parameter
specification as their first argument. An argument list, as from the
application of a user function, is applied to the parameter specification
recursively as follows:

- If the current parameter is a pair, the first argument value is bound to the
  first symbol (see Environments, below), and this process recurs with the
  remainder of the parameters and arguments;
- If the current parameter is a symbol, all remaining arguments are bound to
  that symbol.

It is an error if the first case is reached and either the current parameter or
current argument is an empty list; such errors are "arity" violations. (C
functions may emit arity violations, although such checks are built-in.) Thus,
the length of the parameter list, or whether it is a list, determines the arity
of the function.

For example: `(lambda (a b c) ...)` defines a function with arity exactly 3; it
will fail when applied with less or more than 3 argument values. `(lambda (a b
. c) ...)` defines a function with arity at least 2, where symbol `c` evaluates
to a (possibly empty) list of arguments beyond `b`. `(lambda a ...)` defines a
function with arbitrary arity, where all arguments are assigned to the symbol
`a`. Without loss of generality, this applies to `macro`s as well.

Environments
------------

A TL _environment_ is a proper list of frames; a _frame_ is a proper list of
bindings; and a _binding_ is a pair of a symbol and an object. Symbols, when
evaluated, are looked up in the current environment. Lookup proceeds in frame
order, which means bindings in earlier frames _shadow_ bindings in later
frames.

The built-in `tl-define` creates or updates a binding in the first frame only.
The built-in `tl-set!` will update a binding in _any_ frame, or create it in
the first frame if it does not exist. While `tl-set!` permits greater
flexibility, `tl-define` is more performant, and the use of `tl-define`
encourages further performance gains due to data locality.

The current environment can be accessed with the built-in `tl-env` function
without arguments. When the interpreter begins, this environment has only one
frame; that environment can be retrieved as the value of the `tl-top-env`
function.

User-defined functions (`lambda` or `macro`) store the environment at the time
of their definition. When applied, a new frame is prepended to the stored
environment. In the process of evaluation of a user function:

1. This new frame is set up with the formal parameters bound to the argument
   values (themselves evaluated to direct values if the function is a `lambda`)
   according to the General Parameter process above; and
2. the user code is pushed onto the continuation stack, tail-first, in the new
   environment.

This mechanism allows users to create new scopes dynamically. It will be seen
in Continuations, below, how this can be used to create encapsulation.

A user function, as well as a continuation, can be passed as the sole argument
to `tl-env`; this returns the stored environment of the function or
continuation. `tl-set-env!`, given a function (or continuation) and an
environment, will set the object's stored environment. The stored environment
can be passed to the built-in `tl-eval-in&` to evaluate an expression in that
environment, effectively allowing one to operate within the environment without
invoking or resuming the function or continuation.

Macros, when defined, receive an additional symbol after their parameter
specification, and before their body, called the "environment name". This
symbol is bound to the environment in which the macro was applied. This value
is suitable for use with `tl-eval-in&` to evaluate code in the scope of the
macro's invocation.

Continuations
-------------

TL has first-class support for continuations, which are a value that represents
a suspended computation. The core function is
`tl-call-with-current-continuation` (which `std.tl` binds to the more-brief
`call/cc`), which invokes its argument with a continuation representing the
state of the interpreter as of entering `call/cc`. Typical usage looks as
follows:

```
(call/cc (lambda (k) ...))
```

This expression evaluates, absent any syntax which calls `k`, to the result of
calling the `lambda`, which is usually the tail position of `...`. However, if
`k` is called, such as via `(k 7)`, evaluation _resumes as if_ this particular
`call/cc` had evaluated to `7` instead. Arguments to continuations, like those
to a `lambda`, are evaluated in the current environment before control is
passed.

Within the `lambda` above, this can be interpreted as implementing "early
return"; for example:

```
(call/cc (lambda (return)
  ...
  (if not-worth-continuing
    (return #f)
    ...
  )
))
```

In this example, the second `...` is evaluated if and only if the computation
hasn't been "abandoned" by a true value of `not-worth-continuing` causing the
invocation of `return`. This trick is used to convert the built-in
`tl-eval-in&` to the more straightforward `tl-eval-in` often used in macros:

```
(define tl-eval-in
  (lambda (env ex)
    (call/cc (lambda (ret) (tl-eval-in& env ex ret)))))
```

The `ret` continuation returns to the `call/cc`, which is in the tail position
of `tl-eval-in`.

It's worth noting that `k`, unlike C's `setjmp`/`longjmp`, can outlive the
called function. Indeed,

```
(define current-continuation
  (lambda () (call/cc (lambda (cc) cc))))
```

returns the continuation entered by calling this user function. (The real
definition in `std.tl` is a macro to avoid issues with scoping, but has largely
equivalent behavior.) Used carefully, this continuation can be used as a way to
pass "messages" back into the continuation's scope, not unlike Smalltalk:

```
(define make-object (lambda (state)
  (set! message (current-continuation))
  (cond
    ((= (tl-type message) 'cont) message)
    ((= (cadr message) 'foo) ((car message) ...))
    ((= (cadr message) 'bar) ((car message) ...))
  )
))
```

The frame that holds `state` and `message` as local values is fully held by the
returned continuation; this means that the `...` can use it as mutable state,
where it can be used, for example, to implement private member variables. To be
sure that this continuation escapes, the first `cond` branch uses the internal
`tl-type` function to recognize whether the value is a continuation, and simply
returns it if it is. Otherwise, `cond` dispatches to "methods", recognizes by
their first item; this idiom can be used as follows:

```
(define send-message (lambda (obj . args)
  (call/cc (lambda (ret) (obj (cons ret args))))
))
(define object (make-object 3))
(send-message object 'foo)
(send-message object 'bar 1 2)
(send-message object 'baz object (+ 5 6))
```

In order to avoid escaping back to `(define object ...)`, the methods pass a
"return continuation" as the first element of a list; this is a common pattern
allowing for cooperative coroutines, and moreover allows one to define a
suspendable computation graph independent of the call stack (permitting
scheduling and yielding, a la "async/await" in other languages). Note that the
number of times a continuation can be invoked is arbitrary.

Internally, a continuation is a triple of the continuation stack, the value
stack, and the environment. These three objects are, together, considered to be
"the state" of the interpreter at any given time.
`tl-call-with-current-continuation` captures its invocation state, and then
pushes this object as the sole (direct) value applied to its argument.
Resumption is implemented as a special case in `tl_apply_next`, the
interpreter's "next state" quantum, where it restores the state from the
continuation and pushes its sole expected argument. The value can be direct or
syntactic; if it is syntactic, it is evaluated in a substate (pushed on the
continuation stack).

Continuation and Values Stacks
------------------------------

In fitting with the Call-by-Push-Value mode, TL interpreters maintain two
stacks, `conts` (continuations) and `values`. The `value` stack is, at all
times, a proper list (whose top is the `car`) of pairs of objects and a
"syntax" flag--this is `tl-#t` if the value is syntactic, and `tl-#f` if the
value is direct. (The intepreter has two fields, `true_` and `false_`, that
cache these symbols, so comparison is fast.) The continuation stack is more
complicated, and represents, more or less, a call stack for the running
computation; it, too, is a proper list used as a stack, whose elements are
improper lists of the form `(len expr . env)`. `len` may take some special
values, all of which are negative:

- `TL_APPLY_PUSH_EVAL` (-1): `expr` is a syntax, and `env` is the environment
  in which it shall be evaluated. Usually, this syntax is an application, which
  is handled by `tl_push_eval` by "indirecting" into a more primitive
  application on the continuation stack, as described below.
  
- `TL_APPLY_INDIRECT` (-2): what would ordinarily be `expr` is taken from the
  value stack; it must be a direct callable value. The `expr` position of the
  entry contains the `len` to be used in the application. This is pushed when a
  complex expression needs to evaluate its callable before it can apply it.

- `TL_APPLY_DROP_EVAL` (-3): as `TL_APPLY_PUSH_EVAL`, but the value is silently
  discarded. This is the typical case for non-tail-position user code in a user
  function.

- `TL_APPLY_DROP` (-4): ignores `expr` and `env`, and simply drops the top of
  the value stack. This is emitted in a pair with `TL_APPLY_INDRECT` whenever a
  `TL_APPLY_DROP_EVAL` must be indirected.

- `TL_APPLY_DROP_RESCUE` (-5): ignores `expr` and `env`, and simply drops the
  top of the rescue stack. This is pushed simultaneously with the new rescue
  continuation, and serves to remove it if the computation succeeds.

- `TL_APPLY_GETCHAR` (-6): stops `tl_apply_next`, returning
  `TL_RESULT_GETCHAR`. This is a signal to the top-level (usually
  `tl_run_until_done`, but also any other driver) that more input is needed.
  Allowing the call to `tl_apply_next` to end allows a driver to asynchronously
  restart the computation when more input is needed.

- Otherwise, the entry is a "basic application", where `expr` is a callable
  object, `env` is the environment, and `len` is the number of values it
  expects; `len` values are popped from the value stack, pushing evaluations
  from right to left (such that evaluation order is left to right) if the
  callable expects direct arguments (a `cfunc_byval` or a user `lambda`), or
  raising an error if the callable expects syntax arguments (a `cfunc`, `then`,
  or `macro`) and any argument is a direct value. If collection succeeds, these
  arguments are then passed to the appropriate C function, or to user code as
  the "argument list" described in General Parameters above.

The usual way to evaluate a TL expression from C, then, is to synthesize a
one-argument application:

```c
tl_interp *in = ...;
tl_object *state = ...;
tl_object *expr = ...;
void _c_continuation_k(tl_interp *in, tl_object *args, tl_object *state) { ... }

tl_push_apply(in, 1, tl_new_then(in, _c_continuation_k, state, "_c_continuation_k"), in->env);
tl_push_eval(in, expr, in->env);
```

In this way, the continuation `_c_continuation_k` eventually receives a proper
list of length one, whose `car` is the direct value of the evaluated `expr`.
The interpreter environment `in->env` is assumed to be in a valid state; in a
REPL, this is usually `in->top_env`. The string passed to `tl_new_then` is a
debugging aid. `state` may be `TL_EMPTY_LIST` (or, equivalently, `NULL`) if the
communication of state is not needed.

The idea of pushing applications and evaluable expressions simultaneously
generalizes, of course, to many different applications, many different kinds of
callables, and many different expressions in many different environments, so
long as the stack discipline is observed. In particular, the sum of the `len`
of all applications should equal the number of value pushes done.

It is also possible to spend one redirection to pair an application and its
values directly:

```c
tl_push_apply(in, 1, tl_new_then(in, _c_continuation_k, state, "_c_continuation_k"), in->env);
tl_push_apply(in, TL_APPLY_PUSH_EVAL, expr, in->env);
```

... though more care must be taken to sequence these correctly, as now the
evaluations will be sequenced with respect to the continuation stack; in
particular, the last `TL_APPLY_PUSH_EVAL` will become the first argument. In
essence, this device dynamically arises in the evaluation of user functions,
which appear in place of the `tl_new_then` call above.

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
