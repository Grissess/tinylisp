#ifndef TINYLISP_H
#define TINYLISP_H

#include <stddef.h>

#ifndef NULL
/** Standard NULL. Only defined if not in `stddef.h`. */
#define NULL ((void *) 0)
#endif

#ifndef EOF
/** Standard EOF. Only defined if not in `stdio.h`.
 *
 * You may use this in the `readf` implementation of your interpreter
 * interface.
 */
#define EOF ((int) -1)
#endif

#ifndef TL_DEFAULT_GC_EVENTS
/** The default number of GC events before a collection is attempted.
 *
 * This is 65536 by default, and refers to the number of ::tl_apply_next calls
 * between automatic collections. This will eventually be superseded by a
 * collector cognizant of load.
 */
#define TL_DEFAULT_GC_EVENTS 65536
#endif

#if defined(PTR_LSB_AVAILABLE)
#define TL_FMASK ((1 << PTR_LSB_AVAILABLE) - 1)
#if PTR_LSB_AVAILABLE < 2
#define NO_GC_PACK
#endif
#endif

#if defined(MODULE) && !defined(MODULE_BUILTIN)
#define TL_EXTERN extern
#else
/** `extern` keyword used throughout the header
 *
 * This is set to `extern` whenever the current source (.c) file is being
 * linked as a shared object (DLL), indicating to the compiler that the names
 * in this library are external (because they are in the interpreter which
 * loads them). It is defined as an empty string otherwise.
 */
#define TL_EXTERN
#endif

#ifdef HAVE_SYSTEMTAP
#define SDT_USE_VARIADIC
#include <sys/sdt.h>
#define tl_trace(pt, ...) STAP_PROBEV(tinylisp, pt, __VA_ARGS__)
#else
/** Set the named trace point, optionally with parameters
 *
 * Probes of this kind are intended to indicate "high level events" that may be
 * of interest to performance-monitoring tools. The point should indicate the
 * operation being done; if it bounds a region of time, it is idiomatic to
 * include a "_enter" and "_exit" point. Points like these should be pushed as
 * far down the abstraction (if not necessarily the call stack) as reasonable
 * to improve the accuracy of the instrumentation.
 *
 * In default builds, these expand to nothing (at all!). When `HAVE_SYSTEMTAP`
 * is defined, these compile to `stap` probes that can be instrumented, but by
 * default introduce very little overhead (they compile to a `nop` before being
 * instrumented). `HAVE_DTRACE` support is planned, but not yet supported.
 */
#define tl_trace(pt, ...)
#endif

typedef struct tl_interp_s tl_interp;
typedef struct tl_name_s tl_name;

/** The type of "type tags".
 *
 * These can be attached to application data (::TL_PTR) to indicate an
 * originator. You should get one of these through ::tl_new_tag if you need
 * one.
 */
typedef size_t tl_tag;
/** A reserved value for "no tag" */
#define TL_NO_TAG ((tl_tag)0)

/** Object structure
 *
 * This structure describes every TinyLisp object at runtime, and contains the
 * pertinent members to each type.
 *
 * Note that `NULL` is a valid object&mdash;it is `TL_EMPTY_LIST`, the empty pair
 * `()`.
 */
typedef struct tl_object_s {
	/** The type of this object. Instead of testing this directly, you should prefer the `tl_is_*` macros instead. */
	enum {
		/** TL_INT Integer objects (C long).
		 *
		 * Created by ::tl_new_int .
		 */
		TL_INT,
		/** TL_SYM Symbol objects (loosely, strings, but not C strings).
		 *
		 * Created by ::tl_new_sym or ::tl_new_sym_data .
		 */
		TL_SYM,
		/** TL_PAIR A `cons` pair.
		 *
		 * Created by ::tl_new_pair .
		 */
		TL_PAIR,
		/** A C continuation; the object C code pushes on the stack to defer evaluation.
		 *
		 * Created by ::tl_new_then .
		 */
		TL_THEN,
		/** A built-in function taking arguments by name.
		 *
		 * Created by ::tl_new_cfunc .
		 */
		TL_CFUNC,
		/** A built-in function taking arguments by value.
		 *
		 * Created by ::tl_new_cfunc_byval .
		 */
		TL_CFUNC_BYVAL,
		/** A user-defined function taking arguments (and an environment) by name.
		 *
		 * Created by ::tl_new_macro .
		 */
		TL_MACRO,
		/** A user-defined function taking arguments by value.
		 *
		 * Created by ::tl_new_func .
		 */
		TL_FUNC,
		/** A continuation object; the object passed by `call-with-current-continuation`, which resumes evaluation state.
		 *
		 * Created by ::tl_new_cont .
		 */
		TL_CONT,
		/** A pointer object; an opaque type useful for extension developers.
		 *
		 * Created by ::tl_new_ptr .
		 */
		TL_PTR,
	} kind;
	union {
		/** For ::TL_INT, the signed long integer value. Note that TL does not internally support unlimited precision. */
		long ival;
		/** For ::TL_SYM, the name in the interpreter's namespace trie, comparable by pointer equality. */
		tl_name *nm;
		struct {
			/** For (non-NULL) ::TL_PAIR, a pointer to the first of the pair (CAR in traditional LISP). */
			struct tl_object_s *first;
			/** For (non-NULL) ::TL_PAIR, a pointer to the next of the pair (CDR in traditional LISP). */
			struct tl_object_s *next;
		};
		struct {
			/** For ::TL_THEN and ::TL_CFUNC, a pointer to the actual C function.
			 *
			 * The arguments are:
			 * - the active interpreter (from which the environment can be derived), 
			 * - a TL list of arguments to be applied (evaluated if this is a ::TL_CFUNC_BYVAL ),
			 * - the ::state below, which is usually NULL unless this is a ::TL_THEN .
			 */
			void (*cfunc)(tl_interp *, struct tl_object_s *, struct tl_object_s *);
			/** For ::TL_THEN, the state argument (parameter 3). */
			struct tl_object_s *state;
			/** For ::TL_THEN and ::TL_CFUNC, a C string containing the name of the function, or NULL. */
			char *name;
		};
		struct {
			/** For ::TL_MACRO and ::TL_FUNC, the formal arguments (a linear list of symbols). */
			struct tl_object_s *args;
			/** For ::TL_MACRO and ::TL_FUNC, the body of the function (a linear list of expressions, usually other lists, for which the last provides the valuation). */
			struct tl_object_s *body;
			/** For TL_MACRO and TL_FUNC, the environment captured by the function or macro when it was defined. */
			struct tl_object_s *env;
			/** For TL_MACRO, the TL symbol object containing the name of the argument which will be bound to the evaluation environment, or NULL. */
			struct tl_object_s *envn;
		};
		struct {
			/** For ::TL_CONT, the evaluation environment to which to return. */
			struct tl_object_s *ret_env;
			/** For ::TL_CONT, the return continuation stack to which to return. */
			struct tl_object_s *ret_conts;
			/** For ::TL_CONT, the value stack to which to return. */
			struct tl_object_s *ret_values;
		};
		struct {
			/** For ::TL_PTR, the underlying pointer. */
			void *ptr;
			/** For ::TL_PTR, if not NULL, a function to call on the pointer when the object is garbage-collected (usually to free associated resources).
			 *
			 * This is called whenever the object is GC'd; see ::tl_gc and ::tl_free . Care must be taken if ::ptr can be aliased.
			 */
			void (*gcfunc)(tl_interp *, struct tl_object_s *);
			/** For ::TL_PTR, the tag assigned to this pointer.
			 *
			 * This can be used for source comparisons--see ::tl_tag.
			 */
			tl_tag tag;
		};
	};
#ifdef NO_GC_PACK
	struct tl_object_s *next_alloc;
	struct tl_object_s *prev_alloc;
	unsigned char flags;
#else
	union {
		/** For the garbage collector, a pointer to the next allocated object. */
		struct tl_object_s *next_alloc;
		/** As ::next_alloc but cast to an integer of platform size. Used by the GC for bitpacking. */
		size_t next_alloc_i;
	};
	/** For the garbage collector, a pointer to the previous allocated object. */
	struct tl_object_s *prev_alloc;
#endif
} tl_object;

/** For the garbage collector, the bitmask for bitpacking into tl_object::next_alloc_i .
 *
 * This claims 2 bits of the bottom of the address, which means all objects
 * must be 4-byte aligned. This is the norm on most 32-bit and 64-bit
 * processors (and possibly some others), but may need to be manually tweaked
 * for esoteric platofms.
 */
#ifndef TL_FMASK
#define TL_FMASK 0x3
#endif
/** Mark bit for bitpacking int tl_object::next_alloc_i .
 *
 * The garbage collector uses this to mark objects during the mark pass of its
 * mark/sweep operation.
 */
#define TL_F_MARK 0x1
/** Bit to ask the GC not to collect this object.
 *
 * This bit allows external program to depend on the existence of an object,
 * even if it isn't references from an interpreter root. In effect, this makes
 * the object its own, free-floating root. It is expected that the runtime
 * holds a (per-interpreter) pointer to this, and will free it when it is no
 * longer needed.
 *
 * This property is transitive; objects this refers to will be kept alive as
 * well.
 *
 * Marking an object as permanent does not prevent it from being freed during
 * tl_interp_cleanup(); all objects allocated under the interpreter are
 * destroyed during that time.
 */
#define TL_F_PERMANENT 0x2

#ifdef NO_GC_PACK

#define tl_mark(obj) ((obj)->flags |= TL_F_MARK)
#define tl_unmark(obj) ((obj)->flags &= ~TL_F_MARK)
#define tl_is_marked(obj) ((obj)->flags & TL_F_MARK)
#define tl_make_permanent(obj) ((obj)->flags |= TL_F_PERMANENT)
#define tl_make_transient(obj) ((obj)->flags &= ~TL_F_PERMANENT)
#define tl_is_permanent(obj) ((obj)->flags & TL_F_PERMANENT)
#define tl_next_alloc(obj) ((obj)->next_alloc)
#define tl_make_next_alloc(orig, ptr) (ptr)

#else

/** Mark an object.
 *
 * This sets a bit in the tl_object::next_alloc_i field of *this* object.
 *
 * Only the garbage collector should do this. It clears all marks before doing
 * a mark pass, anyway.
 */
#define tl_mark(obj) ((obj)->next_alloc_i |= TL_F_MARK)
/** Unmark an object.
 *
 * This clears a bit in the tl_object::next_alloc_i field of *this* object.
 *
 * Only the garbage collector should use this.
 */
#define tl_unmark(obj) ((obj)->next_alloc_i &= ~TL_FMASK)
/** Check if an object is marked.
 *
 * This checks a bit in the tl_object::next_alloc_i field of *this* object.
 *
 * This is generally only valid after a mark pass of the garbage collector.
 */
#define tl_is_marked(obj) ((obj)->next_alloc_i & TL_F_MARK)
/** Cause the object to become permanent.
 *
 * This effectively causes the object to behave like its own GC root. See the
 * discussion of ::TL_F_PERMANENT for details.
 */
#define tl_make_permanent(obj) ((obj)->next_alloc_i |= TL_F_PERMANENT)
/** Cause the object to become transient.
 *
 * This unsets permanency, as via ::tl_make_permanent.
 */
#define tl_make_transient(obj) ((obj)->next_alloc_i &= ~TL_F_PERMANENT)
/** Determine whether the object is permanent.
 *
 * This tests the ::TL_F_PERMANENT flag; see it for details.
 */
#define tl_is_permanent(obj) ((obj)->next_alloc_i & TL_F_PERMANENT)
/** Safely dereference the next allocated object.
 *
 * This is necessary because the low bits may have packed flags via
 * tl_object::next_alloc_i .
 */
#define tl_next_alloc(obj) ((tl_object *)((obj)->next_alloc_i & (~TL_FMASK)))
/** Create a new tl_object::next_alloc pointer to store in an object.
 *
 * The value of this macro is suitable to store in tl_object::next_alloc* Pass
 * `orig` as the current value of tl_object::next_alloc , and `ptr` as the
 * pointer to the new object to store as the "next allocation". This macro
 * preserves flags packed into tl_object::next_alloc_i onto this object.
 *
 * Generally, only the allocator needs to do this. Don't forget to keep the
 * doubly-linked-list valid by updating tl_object::prev_alloc as well.
 */
#define tl_make_next_alloc(orig, ptr) ((tl_object *)(((obj)->next_alloc_i & (~TL_FMASK)) | (((size_t)(orig)) & TL_FMASK)))

#endif

TL_EXTERN tl_object *tl_new(tl_interp *);
TL_EXTERN tl_object *tl_new_int(tl_interp *, long);
TL_EXTERN tl_object *tl_new_sym(tl_interp *, const char *);
TL_EXTERN tl_object *tl_new_sym_data(tl_interp *, const char *, size_t);
TL_EXTERN tl_object *tl_new_sym_name(tl_interp *, tl_name *);
TL_EXTERN tl_object *tl_new_pair(tl_interp *, tl_object *, tl_object *);
TL_EXTERN tl_object *tl_new_then(tl_interp *, void (*)(tl_interp *, tl_object *, tl_object *), tl_object *, const char *);
TL_EXTERN tl_object *_tl_new_cfunc(tl_interp *, void (*)(tl_interp *, tl_object *, tl_object *), const char *);
/** Create a new cfunction, taking the name of the C function as the TL function's name */
#define tl_new_cfunc(in, cf) _tl_new_cfunc((in), (cf), #cf)
TL_EXTERN tl_object *_tl_new_cfunc_byval(tl_interp *, void (*)(tl_interp *, tl_object *, tl_object *), const char *);
/** Create a new by-value cfunction, taking the name of the C function as the TL function's name */
#define tl_new_cfunc_byval(in, cf) _tl_new_cfunc_byval((in), (cf), #cf)
TL_EXTERN tl_object *tl_new_macro(tl_interp *, tl_object *, tl_object *, tl_object *, tl_object *);
/** Creates a new lambda (recognized as a macro without an envname) */
#define tl_new_func(in, args, body, env) tl_new_macro((in), (args), NULL, (body), (env))
TL_EXTERN tl_object *tl_new_cont(tl_interp *, tl_object *, tl_object *, tl_object *);
TL_EXTERN tl_object *tl_new_ptr(tl_interp *, void *, void (*)(tl_interp *, tl_object *), tl_tag);
TL_EXTERN void tl_free(tl_interp *, tl_object *);
TL_EXTERN void tl_gc(tl_interp *);

/** Test whether an object is a ::TL_INT. */
#define tl_is_int(obj) ((obj) && (obj)->kind == TL_INT)
/** Test whether an object is a ::TL_SYM. */
#define tl_is_sym(obj) ((obj) && (obj)->kind == TL_SYM)
/* FIXME: NULL is a valid empty list */
/** Test whether an object is a pair.
 *
 * Note that NULL and ::TL_EMPTY_LIST are equivalent, so NULL is a valid
 * pair.
 */
#define tl_is_pair(obj) (!(obj) || (obj)->kind == TL_PAIR)
/** Test whether an object is a ::TL_THEN. */
#define tl_is_then(obj) ((obj) && (obj)->kind == TL_THEN)
/** Test whether an object is a ::TL_CFUNC. */
#define tl_is_cfunc(obj) ((obj) && (obj)->kind == TL_CFUNC)
/** Test whether an object is a ::TL_CFUNC_BYVAL. */
#define tl_is_cfunc_byval(obj) ((obj) && (obj)->kind == TL_CFUNC_BYVAL)
/** Test whether an object is a ::TL_MACRO. */
#define tl_is_macro(obj) ((obj) && (obj)->kind == TL_MACRO)
/** Test whether an object is a ::TL_FUNC. */
#define tl_is_func(obj) ((obj) && (obj)->kind == TL_FUNC)
/** Test whether an object is a ::TL_CONT. */
#define tl_is_cont(obj) ((obj) && (obj)->kind == TL_CONT)
/** Test whether an object is a ::TL_PTR. */
#define tl_is_ptr(obj) ((obj) && (obj)->kind == TL_PTR)
/** Test wither an object is a ::TL_PTR with the appropriate tag. */
#define tl_is_tag(obj, tg) (tl_is_ptr((obj)) && (obj)->tag == (tg))
/** Test whether an object is callable; that is, it can be on the left side of
 *   an application.
 *
 *   Currently, the list includes ::TL_CFUNC, ::TL_CFUNC_BYVAL, ::TL_THEN, ::TL_MACRO, ::TL_FUNC, and ::TL_CONT.
 */
#define tl_is_callable(obj) (tl_is_cfunc(obj) || tl_is_cfunc_byval(obj) || tl_is_then(obj)|| tl_is_macro(obj) || tl_is_func(obj) || tl_is_cont(obj))

/** Get the first of a pair (`car`).
 *
 * If the pair is empty, or the object not a pair, TL_EMPTY_LIST is returned.
 */
#define tl_first(obj) (((obj) && tl_is_pair(obj)) ? (obj)->first : NULL)
/** Get the next of a pair (`cdr`).
 *
 * If the pair is empty, or the object not a pair, TL_EMPTY_LIST is returned.
 */
#define tl_next(obj) (((obj) && tl_is_pair(obj)) ? (obj)->next : NULL)

/** The value of an empty list.
 *
 * Pragmatically, this value is NULL, which means that NULL pointers show up in
 * many structures, and some of TinyLISP is written with this assumption. In
 * effect, NULL := `()`.
 */
#define TL_EMPTY_LIST NULL

/** Iterate over a list.
 *
 * This macro is written to be used inside the parentheses of a for loop,
 * followed by a statement (or block). The argument `obj` is a `TL_PAIR`
 * containing a "proper list" (a linked list of pairs where the first is the
 * value and the next is a pair, or TL_EMPTY_LIST). `it` is a variable name
 * which is defined in the scope as the "current item" of the iterator (the
 * first of each pair). Additionally, `l_it` (the name prefixed with `l_`) is
 * defined as the current pair (such that `tl_first(l_it) == it`).
 */
#define tl_list_iter(obj, it) tl_object *l_##it = obj, *it = tl_first(obj); l_##it; l_##it = tl_next(l_##it), it = tl_first(l_##it)
TL_EXTERN size_t tl_list_len(tl_object *);
TL_EXTERN tl_object *tl_list_rvs(tl_interp *, tl_object *);
TL_EXTERN tl_object *tl_list_rvs_improp(tl_interp *, tl_object *);
TL_EXTERN char *tl_sym_to_cstr(tl_interp *, tl_object *);

/** Determine if two symbols are equal.
 *
 * This checks, in the following order, in theory from least to most expensive:
 * - whether `a` and `b` are both TL_SYM;
 * - whether their names are the same.
 */
#define tl_sym_eq(a, b) (tl_is_sym(a) && tl_is_sym(b) && (a)->nm == (b)->nm)
/** Determine if one symbol is "less than" another.
 *
 * The ordering used here is "shortlex"; `a < b` is implied by, in order:
 * - the length of `a` being less than `b`, or:
 * - if the two symbols are of equal length, whichever has the first byte which
 *   is lesser in numeric value is less.
 *
 * This is a partial order; if `a` and `b` are not TL_SYM, this is always
 * false. When `a` and `b` are restricted to symbols, it is a total order over
 * them. It is never true when `tl_sym_eq(a, b)`.
 */
#define tl_sym_less(a, b) (tl_is_sym(a) && tl_is_sym(b) && ((a)->nm->here.len < (b)->nm->here.len && !((b)->nm->here.len < (a)->nm->here.len) || memcmp((a)->nm->here.data, (b)->nm->here.data, (a)->nm->here.len) < 0))

typedef struct tl_ns_s {
	tl_name *root;
} tl_ns;

/** The interpreter structure.
 *
 * This represents the state of the TinyLISP interpreter at any given point in
 * time. Multiple interpreters can exist simultaneously; TinyLISP is fully
 * reentrant as long as its upcalls (for memory allocation and IO) are.
 *
 * After allocating space for one, an interpreter is initialized using
 * `tl_interp_init` (though some other tasks follow this call; see that
 * function for details). It is finalized with `tl_interp_cleanup`, after which
 * point it is no longer valid. Between these two calls, a library can call any
 * evaluation functions (usually `tl_eval_and_then`), and surrender control to
 * the TinyLISP evaluator to eventually call a continuation with
 * `tl_run_until_done`. See the REPL in `main.c` for an example.
 *
 * TinyLISP code can and does modify many of the fields of this structure (some
 * of which do not make sense outside of the evaluation context provided by
 * `tl_run_until_done`), but it is safe for C code to modify them outside of an
 * evaluation context generally without data races.
 */
struct tl_interp_s {
	/** The symbol namespace.
	 *
	 * This is the owner of a data structure used to accelerate symbol lookups;
	 * whenever a symbol is constructed, this structure is consulted to
	 * efficiently find an object (pointer) unique to the name. Because of this
	 * property, the names so returned can be compared directly for pointer
	 * equality.
	 *
	 * The current implementation of this structure is a variable-width trie
	 * with vector-sorted children.
	 */
	tl_ns ns;
	/** The initial environment of the interpreter.
	 *
	 * This is initialized to an environment containing all of the builtin
	 * top-level functions provided by TinyLISP. See the user guide for this
	 * enumeration.
	 */
	tl_object *top_env;
	/** The current environment of the interpreter.
	 *
	 * This is initialized to point to `top_env`; however, an environment is
	 * pushed whenever a user-defined callable is invoked, and restored by
	 * continuations. This member is most useful from within built-in
	 * functions, where its value represents the current environment (or
	 * namespace) of evaluation.
	 */
	tl_object *env;
	/** The "true" object.
	 *
	 * As an implementation detail, it is the symbol `tl-#t`. It never needs to
	 * be allocated or initialized; for the lifetime of an interpreter, it can
	 * be returned directly from this field.
	 *
	 * Many C functions return this value to indicate they have succeeded, if
	 * they have no other value to return.
	 */
	tl_object *true_;
	/** The "false" object.
	 *
	 * This is, like `true_`, just the symbol `tl-#f`, and likewise does not
	 * need to be allocated.
	 *
	 * Other than predicate functions, this is returned from some C functions
	 * to indicate failure.
	 */
	tl_object *false_;
	/** The current error.
	 *
	 * If a fatal error in evaluation occurs, the evaluator sets this to some
	 * significant value (usually a symbol or a list of some kind, possibly
	 * improper). This generally aborts the entire evaluation back into the C
	 * stack, causing `tl_run_until_done` to return. C interface code should
	 * check the error state via `tl_has_error`; if it is set, the value stack
	 * is likely to be in a less-than-useful state for the invocation.
	 *
	 * When no error has occurred, this field is TL_EMPTY_LIST (or,
	 * equivalently, NULL).
	 */
	tl_object *error;
	/** A registered list of parser prefices.
	 *
	 * The parser (`tl_read`) maintains a list of special characters which can
	 * precede another expression and be translated into an application. This
	 * is, for example, implemented by `std.tl` to turn `'expr` into `(quote
	 * expr)`. Other prefices, including quasiquoting, are implemented this
	 * way, though (as an implementation detail) only one character is allowed
	 * to be used as a prefix.
	 *
	 * The built-in function `tl-prefix` registers a new prefix into this
	 * table. C code can safely do so as well. The list is initially empty, and
	 * is populated as a proper list (in the LISP sense) containing pairs of
	 * symbols (whose first character is exclusively checked) to names (also
	 * symbols), which are substituted into the application `(name expr)` by
	 * the parser at the site in which they are encountered. No other lexical
	 * processing is done.
	 */
	tl_object *prefixes;
	/** The most-recently allocated object.
	 *
	 * This is used by the GC as the first item in a linked list through
	 * allocated objects, to scan the entire set of allocations.
	 */
	tl_object *top_alloc;
	/** The "current continuation"
	 *
	 * This is the continuation popped of the continuation stack (see below).
	 * It lives here mostly so debugging tools can investigate which call
	 * actually caused an error.
	 */
	tl_object *current;
	/** The "continuation stack".
	 *
	 * This is most directly accessed via `tl_push_apply` for pushes, and
	 * popped by `tl_apply_next` (which may, in the course of evaluation,
	 * `tl_push_apply` more continuations.
	 *
	 * One can think of this stack as the "call stack" or "expression stack" of
	 * the interpreter; it consists of callables, intermingled with some
	 * special values which manipulate the value stack. For example, when a
	 * user-declared lambda is evaluated, its body is pushed onto this stack in
	 * reverse order (so that the top is the first expression), and all but the
	 * last expression's values are dropped.
	 *
	 * The data structures inside of this stack are implementation details and
	 * subject to change; full documentation of their structure is outside the
	 * scope of this field's documentation.
	 */
	tl_object *conts;
	/** The "value stack".
	 *
	 * This is most directly accessed by `tl_values_push` (including via
	 * `tl_cfunc_return`) and `tl_values_push_syntactic`, and by the evaluator
	 * using `tl_values_pop_into`. Most C continuations see the values passed
	 * to them as their second arugment, thus not needing to access this stack
	 * directly; if the C function is "by value" (`TL_CFUNC_BYVAL`), the
	 * arguments received for the second parameter consist of the values as
	 * already evaluated (from syntactic to direct).
	 *
	 * The data structure inside of this stack is an implementation detail and
	 * subject to change; however, it is stably a proper LISP list of pairs,
	 * with the first being the value and the second being either `true_` for a
	 * syntactic value or `false_` for a direct value. A discussion of the
	 * difference between syntactic and direct values is outside the scope of
	 * this field's documentation.
	 */
	tl_object *values;
	/** The "rescue stack".
	 *
	 * Each call to the `tl-rescue` builtin pushes a continuation object onto
	 * this stack, which is popped if the callable returns normally. When the
	 * interpreter encounters an error, while unwinding the stack, it will
	 * prefer instead to call the continuation with the error, popping that
	 * continuation off the stack internally. If this stack is empty (the
	 * toplevel context), the error propagates to the application as usual.
	 * (The REPL in `main.c` simply prints the error and grabs the next input
	 * line.)
	 */
	tl_object *rescue;
	/** The number of "events" before `tl_gc` is automatically called by `tl_push_apply`.
	 *
	 * Note that this happens regardless of memory pressure. A smarter
	 * implementation with feedback on memory usage (e.g., through its malloc()
	 * information interface or an OS interface) might wish to call `tl_gc`
	 * manually whenever it experiences memory pressure for better performance.
	 *
	 * To disable automatic GC (and become responsible for calling `tl_gc`
	 * manually), set this to 0.
	 */
	size_t gc_events;
	/** The "event counter" compared to `gc_events`.
	 *
	 * This is incremented by `tl_push_apply`.
	 */
	size_t ctr_events;
	/** The value of the last "putback" (like stdio's ungetc). */
	int putback;
	/** Whether or not `tl_getc` will return the last "putback". */
	int is_putback;
	/** The buffer being built by a `tl_read` call.
	 *
	 * This is stored here for efficiency reasons, and little else; it can't be
	 * part of the state merely because the state has to be GC-able.
	 *
	 * This is safe only because `_tl_read_string_k` and `_tl_read_sym_k` are
	 * not reentrant (per interpreter). Violating this assumption will result
	 * in UB.
	 */
	char *read_buffer;
	/** The current length of the read buffer. */
	size_t read_ptr;
	/** The allocated length of the read buffer. */
	size_t read_sz;
	/** The character `tl-display` writes between arguments.
	 *
	 * This can be set at runtime with tl-display-sep. By default, it is '\t'
	 * (a tab character).
	 */
	char disp_sep;
	/** An opaque "user data" pointer for use with interface functions.
	 *
	 * This value is stored but never modified by TinyLISP; it is not even
	 * initialized by `tl_interp_init`, but this is usually safe since core
	 * TinyLISP never accesses the referent of this pointer.
	 *
	 * It is intended to be accessed from the interface functions, when
	 * TinyLISP needs to "upcall" back into the environment using function
	 * pointers stored in the interpreter.
	 */
	void *udata;
	/** The next tag to return.
	 *
	 * This is the value next returned from ::tl_new_tag, to identify a type.
	 */
	tl_tag next_tag;
	/** Function to read a character immediately.
	 *
	 * This is only invoked by `tl_run_until_done`, using the default handling
	 * of `TL_RESULT_GETCHAR`. If you are using `tl_apply_next` directly, your
	 * handling of `TL_RESULT_GETCHAR` may be different, and this pointer may
	 * never be invoked (and can safely be NULL).
	 *
	 * This is expected to return a C character (a byte) cast to an integer. If
	 * the stream is closed, it is valid to return `EOF` (defined in this or
	 * stdio).
	 *
	 * The argument is the current interpreter.
	 *
	 * An entirely valid implementation relying on stdio can simply `return
	 * getchar()`. The default implementation set by `tl_interp_init` does
	 * this.
	 */
	int (*readf)(struct tl_interp_s *);
	/** Function to write a character.
	 *
	 * This function is called to output a byte of output from TinyLISP to
	 * present to the user.
	 *
	 * The arguments are the current interpreter and the character that is to
	 * be output.
	 *
	 * An entirely valid implementation relying on stdio can simply
	 * `putchar(c)` where `c` is the second argument. The default
	 * implementation set by tl_interp_init() does this.
	 */
	void (*writef)(struct tl_interp_s *, char);
	/** Function to allocate or free memory.
	 *
	 * The arguments are either a pointer previously returned by this function,
	 * or NULL, as well as a desired size. The function should return a pointer
	 * to allocated memory of at least the specified size; when the specified
	 * size is 0, returning NULL is permissible. If the size is greater than 0,
	 * and the argument pointer is not NULL, the region of memory in the
	 * returned pointer of the first size argument bytes or
	 * originally-allocated size bytes (whichever is smaller) should be
	 * equivalent. If the argument size is less than the originally-allocated
	 * size, the returned pointer may be the same (which satisfies this
	 * requirement). The allocator may also expand the area in place, which is
	 * similarly satisfactory. If the returned allocations don't coincide, the
	 * allocator is responsible for copying the memory from the old allocation
	 * before returning it to the free pool.
	 *
	 * These are exactly the POSIX semantics of realloc(), and an entirely
	 * valid implementation may simply call this function in the current libc.
	 *
	 * Pointers given to this function are guaranteed to be previous return
	 * values of this function only when the interpreter was initialized
	 * (before tl_interp_init() returns) with the selfsame function. Otherwise,
	 * implementations may have to expect pointers returned by a previous
	 * implementation. Use tl_interp_init_alloc() to ensure this guarantee.
	 */
	void *(*reallocf)(struct tl_interp_s *, void *, size_t);
#ifdef CONFIG_MODULES
	/** Function to load a module.
	 *
	 * This function is expected to "load a module" into the interpreter.
	 * Naturally, platforms can differ greatly on how this is done, so TinyLISP
	 * leaves most of the details to this function.
	 *
	 * The arguments are the current interpreter and the symbol passed to
	 * `tl-modload` converted to a NUL-terminated C string. The return value is
	 * usually nonzero if it succeeded, or zero if it failed.
	 *
	 * Generally, the modules built in this source tree export a symbol
	 * `tl_init` which is supposed to be called after loading the module into
	 * the current virtual memory space. The value that `tl_init` returns (an
	 * int) can safely be returned from this function. (`tl_init` usually
	 * modifies `top_env` to contain more built-in functions.)
	 * 
	 * Other failures, such as failing to find the module, shouldn't result in
	 * fatal errors (setting `tl_has_error`), but simply return 0.
	 *
	 * A valid implementation for a platform which does not support module
	 * loading may simply always `return 0`. The default implementation set by
	 * tl_interp_init() does this. See the reference REPL in `main.c` for a
	 * UNIX one that uses `dlfcn.h`.
	 */
	int (*modloadf)(struct tl_interp_s *, const char *);
#endif
};

/** Set to indicate the function is `CFUNC_BYVAL` instead of `CFUNC`. */
#define TL_EF_BYVAL 0x01
/** Type of an initialization entry.
 *
 * At compilation time, a number of these entries are put into the same binary
 * section, which the linker then merges (in a specific but unimportant order)
 * into a region which is effectively-addressable as an array.
 * `tl_interp_load_funcs` reads from this array, using it to populate the
 * tl_interp::top_env in ::tl_interp_init.
 */
typedef struct tl_init_ent_s {
	/** The C function to invoke. */
	void (*fn)(tl_interp *, tl_object *, tl_object *);
	/** The name this `CFUNC`/`CFUNC_BYVAL` should have in the environment. */
	const char *name;
	/** `TL_EF` constants, OR'd together. */
	size_t flags;
} __attribute__((aligned(8))) tl_init_ent;

TL_EXTERN void tl_interp_init(tl_interp *);
TL_EXTERN void tl_interp_init_alloc(tl_interp *, void *(*)(struct tl_interp_s *, void *, size_t));
TL_EXTERN void tl_interp_cleanup(tl_interp *);
TL_EXTERN tl_object *tl_interp_load_funcs(tl_interp *, tl_object *, tl_init_ent *, tl_init_ent *);
/** Allocate a new ::tl_tag for use with ::tl_is_tag and ::tl_new_ptr. */
#define tl_new_tag(in) ((in)->next_tag++)

/** Set the error state of the interpreter.
 *
 * As an important implementation detail, the error state cannot be an empty
 * list, which is indistinguishable from "no error".
 *
 * Errors are not well-defined or catalogued, and their exact text is an
 * implementation detail and subject to change. Cautious TinyLISP programs
 * should proactively avoid errors for maximum compatibility. Programs can also
 * raise errors using the `tl-error` builtin function.
 */
#define tl_error_set(in, er) ((in)->error ? (er) : ((in)->error = (er)))
/** Clear the error state of the interpreter. */
#define tl_error_clear(in) ((in)->error = NULL)
/** Evaluates to whether or not the interpreter has an error state. */
#define tl_has_error(in) ((in)->error)

/** Gets a character from the interpreter's input stream.
 *
 * This "immediate mode" API is deprecated, as it is inherently incompatible
 * with asynchronous environments. `tl_run_until_done` is the only user of it
 * by intent; if your environment is asynchronous, implement your own handling
 * of `TL_RESULT_GETCHAR`.
 */
#define tl_getc(in) ((in)->is_putback ? ((in)->is_putback = 0, (in)->putback) : (in)->readf((in)))
/** Put back a character to be read again with `tl_getc`, like `ungetc` in stdio.
 *
 * TinyLISP only stores one putback character at a time.
 */
#define tl_putback(in, c) ((in)->is_putback = 1, (in)->putback = (c))
/** Put a character on the output stream.
 *
 * This usually just invokes tl_interp::writef on the interpreter. The usual C
 * functions are tl_puts(), tl_print(), tl_write(), and tl_printf(), sometimes
 * also called via TinyLISP programs (e.g., `tl-display`).
 */
#define tl_putc(in, c) ((in)->writef((in), (c)))

/** Invoke the interpreter's memory allocation function.
 *
 * See tl_interp::reallocf for details, since this calls that exactly.
 *
 * This macro implements also tl_alloc_malloc() and tl_alloc_free().
 */
#define tl_alloc_realloc(in, p, n) ((in)->reallocf((in), (p), (n)))

/** Invoke the interpreter's malloc function.
 *
 * See tl_interp::reallocf for details. This is called most often via tl_new(),
 * but also called by other routines which must prepare strings (such as
 * ::TL_SYM via tl_new_sym() and tl_read()).
 */
#define tl_alloc_malloc(in, n) tl_alloc_realloc(in, NULL, n)
/** Invoke the interpreter's free function.
 *
 * See tl_interp::reallocf for details.
 *
 * This expression technically has void * result, and the value of this pointer
 * is returned by the allocator, but it is traditionally ignored (and assumed
 * to be NULL) throughout TL.
 */
#define tl_alloc_free(in, ptr) tl_alloc_realloc(in, ptr, 0)

char *tl_strdup(tl_interp *, const char *);
void *tl_calloc(tl_interp *, size_t n, size_t s);

TL_EXTERN tl_object *tl_env_get_kv(tl_interp *, tl_object *, tl_object *);
TL_EXTERN tl_object *tl_env_set_global(tl_interp *, tl_object *, tl_object *, tl_object *);
TL_EXTERN tl_object *tl_env_set_local(tl_interp *, tl_object *, tl_object *, tl_object *);
TL_EXTERN tl_object *tl_frm_set(tl_interp *, tl_object *, tl_object *, tl_object *);
TL_EXTERN tl_object *tl_env_top_pair(tl_interp *);
#define tl_env_top_frame(in) tl_first(tl_env_top_pair((in)))
TL_EXTERN void tl_env_merge(tl_interp *, tl_object *, tl_object *);

TL_EXTERN void tl_cf_macro(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cf_lambda(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cf_define(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_display(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cf_prefix(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_error(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cf_set(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_env(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_setenv(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_topenv(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_type(tl_interp *, tl_object *, tl_object *);

TL_EXTERN void tl_cfbv_cons(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_car(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_cdr(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_null(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cf_if(tl_interp *, tl_object *, tl_object *);

TL_EXTERN void tl_cfbv_concat(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_length(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_ord(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_chr(tl_interp *, tl_object *, tl_object *);

TL_EXTERN void tl_cfbv_readc(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_putbackc(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_writec(tl_interp *, tl_object *, tl_object *);

TL_EXTERN void tl_cfbv_add(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_sub(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_mul(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_div(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_mod(tl_interp *, tl_object *, tl_object *);

TL_EXTERN void tl_cfbv_eq(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_less(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_nand(tl_interp *, tl_object *, tl_object *);

TL_EXTERN void tl_cfbv_load_mod(tl_interp *, tl_object *, tl_object *);

TL_EXTERN tl_object *tl_print(tl_interp *, tl_object *);
TL_EXTERN void tl_puts(tl_interp *, const char *);
TL_EXTERN void tl_write(tl_interp *, const char *, size_t);
TL_EXTERN void tl_printf(tl_interp *, const char *, ...);

/** Push a direct value onto the value stack of the interpreter.
 *
 * Direct values should already be evaluated, and will not be evaluated again
 * if needed in a value position. It is an error for a direct value to appear
 * in a name position.
 */
#define tl_values_push(in, v) (in)->values = tl_new_pair((in), tl_new_pair((in), (v), (in)->false_), (in)->values)
/** Push a syntactic value onto the value stack of the interpreter.
 *
 * Syntactic values may be evaluated if they are found in a value (not name)
 * position.
 */
#define tl_values_push_syntactic(in, v) (in)->values = tl_new_pair((in), tl_new_pair((in), (v), (in)->true_), (in)->values)
/** Pop a value off the value stack and into a `tl_object *` variable named `var`.
 *
 * This routine is deprecated because it discards the syntactic/direct flag.
 */
#define tl_values_pop_into(in, var) do { \
	var = tl_first(tl_first((in)->values)); \
	(in)->values = tl_next((in)->values); \
} while(0)
/** Push a rescue continuation onto the rescue stack.
 *
 * The arguments should be an interpreter and a continuation object (but this
 * is not checked). The top continuation on this stack is called,
 * preferentially, during a stack unwind when the interpreter encounters an
 * error, if present.
 */
#define tl_rescue_push(in, cont) (in)->rescue = tl_new_pair((in), (cont), (in)->rescue);
/** Peek at the top continuation of the rescue stack.
 *
 * This should return either a continuation object, or NULL.
 */
#define tl_rescue_peek(in) tl_first((in)->rescue)
/** Drop the top continuation of the rescue stack.
 *
 * Note that this macro does not return the continuation; use it after
 * tl_rescue_peek.
 */
#define tl_rescue_drop(in) (in)->rescue = tl_next((in)->rescue)
/** Return a value from a C function.
 *
 * All C functions must return a value (`true_` is a good candidate if no other
 * value makes sense). This macro encapsulates both pushing that value onto the
 * value stack and ending control flow (it contains `return`).
 *
 * Although the C compiler might not catch it, anything after this invocation
 * is dead code.
 */
#define tl_cfunc_return(in, v) do { tl_values_push((in), (v)); return; } while(0)
TL_EXTERN int tl_push_eval(tl_interp *, tl_object *, tl_object *);
/** A special continuation flag for pushing an expression to be evaluated onto the stack.
 *
 * This is specifically done with the tail-position expression for a
 * user-defined callable.
 */
#define TL_APPLY_PUSH_EVAL -1
/** A special continuation flag for popping the callable off the value stack.
 *
 * When tl_push_eval returns true, it indicates that an application to be
 * evaluated needs another evaluation to determine the callable (e.g., `((begin
 * display) 7)`). To mark this case, `tl_apply_next` puts this value on the
 * continuation stack to indicate that the value pushed for the next evaluation
 * (queued by `tl_push_apply`) is to be used as a callable for the next
 * application. The current application is said to be "suspended", and will be
 * "resumed" after the callable value is available. (Note that the current
 * application may never resume if this stack is abandoned via resuming another
 * continuation directly--calling a TL_CONT object.)
 *
 * This indirection trick is also used in the implementation of the apply
 * builtin.
 */
#define TL_APPLY_INDIRECT -2
/** A special continuation flag for evaluating, but not pushing, an evaluation of an expression.
 *
 * This is specifically done with all non-tail-position expressions for a
 * user-defined callable.
 */
#define TL_APPLY_DROP_EVAL -3
/** A special continuation flag that simply drops the top of the value stack and continues.
 *
 * This needs to be left when a TL_APPLY_DROP_EVAL evaluation is indirected,
 * which is indeed a rare circumstance.
 */
#define TL_APPLY_DROP -4
/** A special continuation flag that drops the top of the rescue stack.
 *
 * This is left behind by `tl-rescue` to indicate when its callable is
 * finished.
 */
#define TL_APPLY_DROP_RESCUE -5
/** A special continuation flag that immediately results in TL_RESULT_GETCHAR.
 *
 * This is used by `tl_getc_and_then` to get the actual input.
 */
#define TL_APPLY_GETCHAR -6
/** An application result that indicates that the program is done evaluating.
 *
 * `tl_apply_next` returns this when nothing is left to do. Note that it is the
 * only false value.
 */
#define TL_RESULT_DONE 0
/** An application result that indicates the program is still running.
 *
 * `tl_apply_next` returns this regularly during normal evaluation steps,
 * mostly to clear the C stack ("trampoline") before considering the next
 * application.
 *
 * If this value is encountered, simply call `tl_apply_next` again.
 */
#define TL_RESULT_AGAIN 1
/** An application result that indicates the program is suspended on input.
 *
 * `tl_apply_next` returns this whenever a program seeks to read an input byte.
 * Before resuming, it assumes the input byte has been pushed as a `TL_INT` as
 * a direct value one the stack. (Technically, via `tl_eval_and_then`, a
 * syntactic value can be pushed to be evaluated, but this isn't expected to be
 * typical for simply getting a byte from input.)
 *
 * If this value is encountered, make sure to `tl_values_push` a `tl_new_int`
 * with the read input before calling `tl_apply_next` again.
 */
#define TL_RESULT_GETCHAR 2
TL_EXTERN void tl_push_apply(tl_interp *, long, tl_object *, tl_object *);
TL_EXTERN int tl_apply_next(tl_interp *);
TL_EXTERN void _tl_eval_and_then(tl_interp *, tl_object *, tl_object *, void (*)(tl_interp *, tl_object *, tl_object *), const char *);
/** Invokes `_tl_eval_and_then` with the stringified name of the callback. */
#define tl_eval_and_then(in, ex, st, cb) _tl_eval_and_then((in), (ex), (st), (cb), "tl_eval_and_then:" #cb)
TL_EXTERN void _tl_getc_and_then(tl_interp *, tl_object *, void (*)(tl_interp *, tl_object *, tl_object *), const char *);
/** Invokes `_tl_getc_and_then` with the stringified name of the callback. */
#define tl_getc_and_then(in, st, cb) _tl_getc_and_then((in), (st), (cb), "tl_getc_and_then:" #cb)
TL_EXTERN void _tl_eval_all_args(tl_interp *, tl_object *, tl_object *, void (*)(tl_interp *, tl_object *, tl_object *), const char *);
/** Invokes `_tl_eval_all_args` with the stringified name of the callback. */
#define tl_eval_all_args(in, args, state, cb) _tl_eval_all_args((in), (args), (state), (cb), "tl_eval_all_args:" #cb)

TL_EXTERN void tl_cfbv_evalin(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_call_with_current_continuation(tl_interp *, tl_object *, tl_object *);
/* tl_object *tl_cf_apply(tl_interp *, tl_object *); */
void tl_run_until_done(tl_interp *);

TL_EXTERN void tl_read(tl_interp *);
/** Reads an expression, then invokes the continuation with it as its only argument. */
#define tl_read_and_then(in, cb, st) do { \
	tl_push_apply((in), 1, tl_new_then((in), (cb), (st), "tl_read_and_then:" #cb), (in)->env); \
	tl_read(in); \
} while(0)

TL_EXTERN void tl_cfbv_read(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_gc(tl_interp *, tl_object *, tl_object *);

#ifdef DEBUG
TL_EXTERN void tl_dbg_print(tl_object *, int);
TL_EXTERN void tl_cf_debug_print(tl_interp *, tl_object *, tl_object *);
#endif

#ifdef MODULE

#ifdef MODULE_BUILTIN

#define TL_MOD_INIT static int tl_init(tl_interp *, const char *); \
static void * __attribute__((section("tl_bmcons"))) tl_init_fp = tl_init; \
static int tl_init

#else

#define TL_MOD_INIT int tl_init

#endif  /* ifdef MODULE_BUILTIN */

#endif  /* ifdef MODULE */

#define TL_START_INIT_ENTS_NAME __start_tl_init_ents
#define TL_STOP_INIT_ENTS_NAME __stop_tl_init_ents
extern tl_init_ent TL_START_INIT_ENTS_NAME, TL_STOP_INIT_ENTS_NAME;
#define TL_START_INIT_ENTS &TL_START_INIT_ENTS_NAME
#define TL_STOP_INIT_ENTS &TL_STOP_INIT_ENTS_NAME

/** Define a ::tl_init_ent .
 *
 * This has the effect of asking the linker to add a "variable" named
 * `init_tl_cf_`*nm* to the `tl_init_ents` section. On GNU and LLVM linkers,
 * this results in a symbol called __start_tl_init_ents and __stop_tl_init_ents
 * pointing to the first and just past the end of all variables assembled
 * there, allowing them to be loaded as an array into ::tl_interp_load_funcs.
 *
 * By using this macro, you will have this section in your binary. If you are
 * linking statically against TL, all such entries will be automatically loaded
 * into any interpreter by way of ::tl_interp_init. (See, e.g., `main.c`, which
 * adds some utility functions to the basic REPL that are available in this
 * manner.) If you are linking dynamically, you may need some linker-specific
 * calls to extract these sections from the shared object, and they won't
 * automatically be called on interpreter init (since loads can happen much
 * later). If you're authoring TL modules, a fine time to load these is in the
 * function declared by ::TL_MOD_INIT .
 *
 * Use ::tl_interp_load_funcs with the pointers to these sections to load the
 * entries. (This is ultimately what ::tl_interp_init does.)
 */
#define TL_CF_FLAGS(func, nm, f) void tl_cf_##func(tl_interp *, tl_object *, tl_object *);\
static tl_init_ent __attribute__((section("tl_init_ents"),aligned(8),used)) init_tl_cf_##func = {\
	.fn = tl_cf_##func, .name = "tl-" nm, .flags = (f),\
};\
void tl_cf_##func(tl_interp *in, tl_object *args, tl_object *_)
/** Declare a CFUNC.
 *
 * `func` must be a syntactically-valid C identifier, to which other fragments of identifier will be prepended. `nm` is the name to which the function will be bound in the interpreter's tl_interp::top_env.
 *
 * This macro expands to a prototype, a declaration, and the beginning of a
 * function definition, such that it is suitable to follow by a function body
 * in a brace block. The name and arguments are declared as follows:
 *
 *		`void tl_cf_`*nm*`(tl_interp *in, tl_object *args, tl_object *_)`
 *
 * (The `_` argument is a "state" argument, which is always `NULL` for
 * non-continuations, but a necessary part of the function type.)
 */
#define TL_CF(func, nm) TL_CF_FLAGS(func, nm, 0)
/** Declare a CFUNC_BYVAL.
 *
 * Exactly the same as `TL_CF`, but the underlying function is
 * `TL_CFUNC_BYVAL`, which will receive all of its arguments by value, instead
 * of by name as default.
 *
 * Since eval is a one-way arrow (from syntax to value), `TL_CFUNC_BYVAL` is
 * generally easier to invoke. It should be preferred unless it can't be
 * avoided, including for nilary functions (with no expected arguments).
 */
#define TL_CFBV(func, nm) TL_CF_FLAGS(func, nm, TL_EF_BYVAL)

#if defined(MODULE) && !defined(MODULE_BUILTIN)
#define TL_LOAD_FUNCS do { \
	tl_object *frm = NULL; \
	frm = tl_interp_load_funcs(in, frm, TL_START_INIT_ENTS, TL_STOP_INIT_ENTS); \
	tl_env_merge(in, tl_env_top_pair(in), frm); \
} while(0)
#else
/** Load the `TL_CF` and `TL_CFBV` entries.
 *
 * Call this from `TL_MOD_INIT`. If the module is linked externally, this
 * merges the environment. If it's statically linked, this does nothing--the
 * entries would've been loaded by ::tl_interp_init anyway.
 */
#define TL_LOAD_FUNCS
#endif

#define tl_min(x, y) ((x) < (y) ? (x) : (y))

typedef struct tl_buffer_s {
	char *data;
	size_t len;
} tl_buffer;

typedef struct tl_child_s {
	tl_buffer seg;
	struct tl_name_s *name;
} tl_child;

struct tl_name_s {
	tl_buffer here;
	size_t num_children;
	size_t sz_children;
	tl_child *children;
	struct tl_name_s *chain;
};

void tl_ns_init(tl_interp *, tl_ns *);
void tl_ns_free(tl_interp *, tl_ns *);
tl_name *tl_ns_resolve(tl_interp *, tl_ns *, tl_buffer);
void tl_ns_print(tl_interp *, tl_ns *);
void tl_ns_for_each(tl_interp *, tl_ns *, void (*)(tl_interp *, tl_ns *, tl_name *, void *), void *);
tl_buffer tl_buf_slice(tl_interp *, tl_buffer, size_t, size_t);

#endif
