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

#if defined(MODULE) && !defined(MODULE_BUILTIN)
#define TL_EXTERN extern
#else
#define TL_EXTERN
#endif

typedef struct tl_interp_s tl_interp;

/** Object structure
 *
 * This structure describes every TinyLisp object at runtime, and contains the
 * pertinent members to each type.
 *
 * Note that `NULL` is a valid object--it is `TL_EMPTY_LIST`, the empty pair
 * `()`.
 */
typedef struct tl_object_s {
	/** The type of this object. Instead of testing this directly, you should prefer the `tl_is_*` macros instead. */
	enum {
		TL_INT,
		TL_SYM,
		TL_PAIR,
		TL_THEN,
		TL_CFUNC,
		TL_CFUNC_BYVAL,
		TL_MACRO,
		TL_FUNC,
		TL_CONT,
	} kind;
	union {
		/** For `TL_INT`, the signed long integer value. Note that TL does not internally support unlimited precision. */
		long ival;
		struct {
			/** For `TL_SYM`, a pointer to a byte array containing the symbol name. The pointed-to memory should be treated as read-only&mdash;if you need a new symbol, make another. */
			char *str;
			/** For `TL_SYM`, the length of the pointed-to byte array. */
			size_t len;
			/** For `TL_SYM`, a hash of the symbol, used to speed up comparison. */
			unsigned long hash;
		};
		struct {
			/** For (non-NULL) `TL_PAIR`, a pointer to the first of the pair (CAR in traditional LISP). */
			struct tl_object_s *first;
			/** For (non-NULL) `TL_PAIR`, a pointer to the next of the pair (CDR in traditional LISP). */
			struct tl_object_s *next;
		};
		struct {
			/** For `TL_THEN` and `TL_CFUNC`, a pointer to the actual C function. */
			void (*cfunc)(tl_interp *, struct tl_object_s *, struct tl_object_s *);
			/** For `TL_THEN`, the state argument (parameter 3). */
			struct tl_object_s *state;
			/** For `TL_THEN` and `TL_CFUNC`, a C string containing the name of the function, or `NULL`. */
			char *name;
		};
		struct {
			/** For `TL_MACRO` and `TL_FUNC`, the formal arguments (a linear list of symbols). */
			struct tl_object_s *args;
			/** For `TL_MACRO` and `TL_FUNC`, the body of the function (a linear list of expressions, usually other lists, for which the last provides the valuation). */
			struct tl_object_s *body;
			/** For `TL_MACRO` and `TL_FUNC`, the environment captured by the function or macro when it was defined. */
			struct tl_object_s *env;
			/** For `TL_MACRO`, the TL symbol object containing the name of the argument which will be bound to the evaluation environment, or `NULL`. */
			struct tl_object_s *envn;
		};
		struct {
			/** For `TL_CONT`, the evaluation environment to which to return. */
			struct tl_object_s *ret_env;
			/** For `TL_CONT`, the return continuation stack to which to return. */
			struct tl_object_s *ret_conts;
			/** For `TL_CONT`, the value stack to which to return. */
			struct tl_object_s *ret_values;
		};
	};
	union {
		/** For the garbage collector, a pointer to the next allocated object. */
		struct tl_object_s *next_alloc;
		/** As `next_alloc` but cast to an integer of platform size. Used by the GC for bitpacking. */
		size_t next_alloc_i;
	};
	/** For the garbage collector, a pointer to the previous allocated object. */
	struct tl_object_s *prev_alloc;
} tl_object;

/** For the garbage collector, the bitmask for bitpacking into :ref:`next_alloc_i`. 
 *
 * This claims 2 bits of the bottom of the address, which means all objects
 * must be 4-byte aligned. This is the norm on most 32-bit and 64-bit
 * processors (and possibly some others), but may need to be manually tweaked
 * for esoteric platofms.
 */
#define TL_FMASK 0x3
/** Mark bit for bitpacking int :ref:`next_alloc_i`.
 *
 * The garbage collector uses this to mark objects during the mark pass of its
 * mark/sweep operation.
 */
#define TL_F_MARK 0x1

/** Mark an object.
 *
 * This sets a bit in the :ref:`next_alloc_i` field of *this* object.
 *
 * Only the garbage collector should do this. It clears all marks before doing
 * a mark pass, anyway.
 */
#define tl_mark(obj) ((obj)->next_alloc_i |= TL_F_MARK)
/** Unmark an object.
 *
 * This clears a bit in the :ref:`next_alloc_i` field of *this* object.
 *
 * Only the garbage collector should use this.
 */
#define tl_unmark(obj) ((obj)->next_alloc_i &= ~TL_FMASK)
/** Check if an object is marked.
 *
 * This checks a bit in the :ref:`next_alloc_i` field of *this* object.
 *
 * This is generally only valid after a mark pass of the garbage collector.
 */
#define tl_is_marked(obj) ((obj)->next_alloc_i & TL_F_MARK)
/** Safely dereference the next allocated object.
 *
 * This is necessary because the low bits may have packed flags via
 * :ref:`next_alloc_i`.
 */
#define tl_next_alloc(obj) ((tl_object *)((obj)->next_alloc_i & (~TL_FMASK)))
/** Create a new `next_alloc` pointer to store in an object.
 *
 * The value of this macro is suitable to store in :ref:`next_alloc`.
 *
 * Pass `orig` as the current value of :ref:`next_alloc`, and `ptr` as the pointer
 * to the new object to store as the "next allocation". This macro preserves
 * flags packed into :ref:`next_alloc_i` onto this object.
 *
 * Generally, only the allocator needs to do this. Don't forget to keep the
 * doubly-linked-list valid by updating :ref:`prev_alloc` as well.
 */
#define tl_make_next_alloc(orig, ptr) ((tl_object *)(((obj)->next_alloc_i & (~TL_FMASK)) | (((size_t)(orig)) & TL_FMASK)))

TL_EXTERN tl_object *tl_new(tl_interp *);
TL_EXTERN tl_object *tl_new_int(tl_interp *, long);
TL_EXTERN tl_object *tl_new_sym(tl_interp *, const char *);
TL_EXTERN tl_object *tl_new_sym_data(tl_interp *, const char *, size_t);
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
TL_EXTERN void tl_free(tl_interp *, tl_object *);
TL_EXTERN void tl_gc(tl_interp *);

#define tl_is_int(obj) ((obj) && (obj)->kind == TL_INT)
#define tl_is_sym(obj) ((obj) && (obj)->kind == TL_SYM)
/* FIXME: NULL is a valid empty list */
/** Test whether an object is a pair. Note that NULL and TL_EMPTY_LIST are
 *   equivalent, so NULL is a valid pair.
 */
#define tl_is_pair(obj) (!(obj) || (obj)->kind == TL_PAIR)
#define tl_is_then(obj) ((obj) && (obj)->kind == TL_THEN)
#define tl_is_cfunc(obj) ((obj) && (obj)->kind == TL_CFUNC)
#define tl_is_cfunc_byval(obj) ((obj) && (obj)->kind == TL_CFUNC_BYVAL)
#define tl_is_macro(obj) ((obj) && (obj)->kind == TL_MACRO)
#define tl_is_func(obj) ((obj) && (obj)->kind == TL_FUNC)
#define tl_is_cont(obj) ((obj) && (obj)->kind == TL_CONT)
#define tl_is_callable(obj) (tl_is_cfunc(obj) || tl_is_cfunc_byval(obj) || tl_is_then(obj)|| tl_is_macro(obj) || tl_is_func(obj) || tl_is_cont(obj))

#define tl_first(obj) (((obj) && tl_is_pair(obj)) ? (obj)->first : NULL)
#define tl_next(obj) (((obj) && tl_is_pair(obj)) ? (obj)->next : NULL)

#define TL_EMPTY_LIST NULL

#define tl_list_iter(obj, it) tl_object *l_##it = obj, *it = tl_first(obj); l_##it; l_##it = tl_next(l_##it), it = tl_first(l_##it)
TL_EXTERN size_t tl_list_len(tl_object *);
TL_EXTERN tl_object *tl_list_rvs(tl_interp *, tl_object *);

#define TL_HASH_LINEAR 16
TL_EXTERN unsigned long tl_data_hash(const char *, size_t);

#define tl_sym_eq(a, b) (tl_is_sym(a) && tl_is_sym(b) && (a)->len == (b)->len && (a)->hash == (b)->hash && !memcmp(a->str, b->str, a->len))
#define tl_sym_less(a, b) (tl_is_sym(a) && tl_is_sym(b) && ((a)->len < (b)->len || memcmp((a)->str, (b)->str, (a)->len) < 0))

struct tl_interp_s {
	tl_object *top_env;
	tl_object *env;
	tl_object *true_;
	tl_object *false_;
	tl_object *error;
	tl_object *prefixes;
	tl_object *top_alloc;
	tl_object *conts;
	tl_object *values;
	size_t gc_events;
	size_t ctr_events;
	int putback;
	int is_putback;
	void *udata;
	int (*readf)(void *, struct tl_interp_s *);
	void (*writef)(void *, struct tl_interp_s *, char);
#ifdef CONFIG_MODULES
	int (*modloadf)(void *, struct tl_interp_s *, const char *);
#endif
};

TL_EXTERN void tl_interp_init(tl_interp *);
TL_EXTERN void tl_interp_cleanup(tl_interp *);

#define tl_error_set(in, er) ((in)->error ? (er) : ((in)->error = (er)))
#define tl_error_clear(in) ((in)->error = NULL)
#define tl_has_error(in) ((in)->error)

#define tl_getc(in) ((in)->is_putback ? ((in)->is_putback = 0, (in)->putback) : (in)->readf((in)->udata, (in)))
#define tl_putback(in, c) ((in)->is_putback = 1, (in)->putback = (c))
#define tl_putc(in, c) ((in)->writef((in)->udata, (in), (c)))

TL_EXTERN tl_object *tl_env_get_kv(tl_interp *, tl_object *, tl_object *);
TL_EXTERN tl_object *tl_env_set_global(tl_interp *, tl_object *, tl_object *, tl_object *);
TL_EXTERN tl_object *tl_env_set_local(tl_interp *, tl_object *, tl_object *, tl_object *);
TL_EXTERN tl_object *tl_frm_set(tl_interp *, tl_object *, tl_object *, tl_object *);

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

#define tl_values_push(in, v) (in)->values = tl_new_pair((in), tl_new_pair((in), (v), (in)->false_), (in)->values)
#define tl_values_push_syntactic(in, v) (in)->values = tl_new_pair((in), tl_new_pair((in), (v), (in)->true_), (in)->values)
#define tl_values_pop_into(in, var) do { \
	var = tl_first(tl_first((in)->values)); \
	(in)->values = tl_next((in)->values); \
} while(0)
#define tl_cfunc_return(in, v) do { tl_values_push((in), (v)); return; } while(0)
TL_EXTERN int tl_push_eval(tl_interp *, tl_object *, tl_object *);
#define TL_APPLY_PUSH_EVAL -1
#define TL_APPLY_INDIRECT -2
#define TL_APPLY_DROP_EVAL -3
#define TL_APPLY_DROP -4
TL_EXTERN void tl_push_apply(tl_interp *, long, tl_object *, tl_object *);
TL_EXTERN int tl_apply_next(tl_interp *);
TL_EXTERN void _tl_eval_and_then(tl_interp *, tl_object *, tl_object *, void (*)(tl_interp *, tl_object *, tl_object *), const char *);
#define tl_eval_and_then(in, ex, st, cb) _tl_eval_and_then((in), (ex), (st), (cb), "tl_eval_and_then:" #cb)
TL_EXTERN void _tl_eval_all_args(tl_interp *, tl_object *, tl_object *, void (*)(tl_interp *, tl_object *, tl_object *), const char *);
#define tl_eval_all_args(in, args, state, cb) _tl_eval_all_args((in), (args), (state), (cb), "tl_eval_all_args:" #cb)
#define tl_run_until_done(in) while(tl_apply_next((in)))

TL_EXTERN void tl_cfbv_evalin(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_call_with_current_continuation(tl_interp *, tl_object *, tl_object *);
/* tl_object *tl_cf_apply(tl_interp *, tl_object *); */

TL_EXTERN tl_object *tl_read(tl_interp *, tl_object *);

TL_EXTERN void tl_cfbv_read(tl_interp *, tl_object *, tl_object *);
TL_EXTERN void tl_cfbv_gc(tl_interp *, tl_object *, tl_object *);

#ifdef DEBUG
TL_EXTERN void tl_dbg_print(tl_object *, int);
TL_EXTERN void tl_cf_debug_print(tl_interp *, tl_object *, tl_object *);
#endif

#ifdef MODULE

#ifdef MODULE_BUILTIN

#define TL_MOD_INIT static int tl_init(tl_interp *, const char *); \
static void * __attribute__((section(".tl_bmcons"))) tl_init_fp = tl_init; \
static int tl_init

#else

#define TL_MOD_INIT int tl_init

#endif  /* ifdef MODULE_BUILTIN */

#endif  /* ifdef MODULE */

#endif
