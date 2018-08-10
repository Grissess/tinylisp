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

Running
-------

`make run` in the repository root. Internally, this rule runs:

	cat std.tl - | $(DEBUGGER) ./tl

...which loads the `std.tl` "standard" library to provide access to more common
LISP functionality. It is worth noting that the majority of TinyLISP's language
is implemented in itself, using its powerful metaprogramming capabilities;
refer to the language specification in the documentation.

Embedding
---------

Refer to `main.c` for a typical way to pass control to a running interpreter.
In general, the major interface will be the pair `tl_eval_and_then` to
initialize the evaluation of an expression, followed by `tl_run_until_done` to
crank the interpreter until the final continuation is called.

Language
--------

`tl_read` in `read.c` is the lexical analyzer. In short, a lexeme may be:

- `"` (double quote) bounds a symbol, which may contain any character other than `"`;
- `()` (parentheses) bound a proper list of lexemes;
- all digits introduce decimal numbers;
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

License
-------

TinyLISP is distributed under the terms of the Apache 2.0 License. See
`COPYING` for more information.

Support
-------

Please feel free to leave issues on this repository, or submit pull requests. I
will make every effort to respond to them in a timely fashion.
