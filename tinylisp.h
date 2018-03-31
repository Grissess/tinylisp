#ifndef TINYLISP_H
#define TINYLISP_H

#include <stddef.h>

#ifndef NULL
#define NULL ((void *) 0)
#endif

#ifndef EOF
#define EOF ((int) -1)
#endif

typedef struct tl_interp_s tl_interp;

typedef struct tl_object_s {
	enum {
		TL_INT,
		TL_SYM,
		TL_PAIR,
		TL_CFUNC,
		TL_MACRO,
		TL_FUNC,
		TL_CONT,
	} kind;
	union {
		long ival;
		char *str;
		struct {
			struct tl_object_s *first;
			struct tl_object_s *next;
		};
		struct {
			void (*cfunc)(tl_interp *, struct tl_object_s *, void *);
			void *state;
			char *name;
		};
		struct {
			struct tl_object_s *args;
			struct tl_object_s *body;
			struct tl_object_s *env;
			char *envn;
		};
		struct {
			struct tl_object_s *ret_env;
			struct tl_object_s *ret_cont;
			struct tl_object_s *ret_values;
		};
	};
	union {
		struct tl_object_s *next_alloc;
		size_t next_alloc_i;
	};
	struct tl_object_s *prev_alloc;
} tl_object;

#define TL_FMASK 0x3
#define TL_F_MARK 0x1

#define tl_mark(obj) ((obj)->next_alloc_i |= TL_F_MARK)
#define tl_unmark(obj) ((obj)->next_alloc_i &= ~TL_FMASK)
#define tl_is_marked(obj) ((obj)->next_alloc_i & TL_F_MARK)
#define tl_next_alloc(obj) ((tl_object *)((obj)->next_alloc_i & (~TL_FMASK)))
#define tl_make_next_alloc(orig, ptr) ((tl_object *)(((obj)->next_alloc_i & (~TL_FMASK)) | (((size_t)(orig)) & TL_FMASK)))

tl_object *tl_new(tl_interp *);
tl_object *tl_new_int(tl_interp *, long);
tl_object *tl_new_sym(tl_interp *, const char *);
tl_object *tl_new_pair(tl_interp *, tl_object *, tl_object *);
tl_object *tl_new_then(tl_interp *, void (*)(tl_interp *, tl_object *, void *), void *, const char *);
#define tl_new_cfunc(in, cf) tl_new_then((in), (cf), NULL, #cf)
tl_object *tl_new_macro(tl_interp *, tl_object *, const char *, tl_object *, tl_object *);
#define tl_new_func(in, args, body, env) tl_new_macro((in), (args), NULL, (body), (env))
tl_object *tl_new_cont(tl_interp *, tl_object *, tl_object *, tl_object *);
void tl_free(tl_interp *, tl_object *);
void tl_gc(tl_interp *);

#define tl_is_int(obj) ((obj) && (obj)->kind == TL_INT)
#define tl_is_sym(obj) ((obj) && (obj)->kind == TL_SYM)
/* FIXME: NULL is a valid empty list */
#define tl_is_pair(obj) (!(obj) || (obj)->kind == TL_PAIR)
#define tl_is_cfunc(obj) ((obj) && (obj)->kind == TL_CFUNC)
#define tl_is_macro(obj) ((obj) && (obj)->kind == TL_MACRO)
#define tl_is_func(obj) ((obj) && (obj)->kind == TL_FUNC)
#define tl_is_cont(obj) ((obj) && (obj)->kind == TL_CONT)
#define tl_is_callable(obj) (tl_is_cfunc(obj) || tl_is_macro(obj) || tl_is_func(obj) || tl_is_cont(obj))

#define tl_first(obj) (((obj) && tl_is_pair(obj)) ? (obj)->first : NULL)
#define tl_next(obj) (((obj) && tl_is_pair(obj)) ? (obj)->next : NULL)

#define TL_EMPTY_LIST NULL

#define tl_list_iter(obj, it) tl_object *l_##it = obj, *it = tl_first(obj); l_##it; l_##it = tl_next(l_##it), it = tl_first(l_##it)
size_t tl_list_len(tl_object *);
tl_object *tl_list_rvs(tl_interp *, tl_object *);

typedef struct tl_interp_s {
	tl_object *top_env;
	tl_object *env;
	tl_object *true_;
	tl_object *false_;
	tl_object *error;
	tl_object *prefixes;
	tl_object *top_alloc;
	tl_object *conts;
	tl_object *values;
	void *udata;
	int (*readf)(void *);
	void (*putbackf)(void *, int);
	void (*printf)(void *, const char *, ...);
} tl_interp;

void tl_interp_init(tl_interp *);
void tl_interp_cleanup(tl_interp *);

#define tl_error_set(in, er) ((in)->error ? (er) : ((in)->error = (er)))
#define tl_error_clear(in) ((in)->error = NULL)
#define tl_has_error(in) ((in)->error)

tl_object *tl_env_get_kv(tl_object *, const char *);
tl_object *tl_env_get(tl_object *, const char *);
tl_object *tl_env_set_global(tl_interp *, tl_object *, const char *, tl_object *);
tl_object *tl_env_set_local(tl_interp *, tl_object *, const char *, tl_object *);
tl_object *tl_frm_set(tl_interp *, tl_object *, const char *, tl_object *);

void tl_cf_macro(tl_interp *, tl_object *, void *);
void tl_cf_lambda(tl_interp *, tl_object *, void *);
void tl_cf_define(tl_interp *, tl_object *, void *);
void tl_cf_display(tl_interp *, tl_object *, void *);
void tl_cf_prefix(tl_interp *, tl_object *, void *);
void tl_cf_error(tl_interp *, tl_object *, void *);
void tl_cf_set(tl_interp *, tl_object *, void *);
void tl_cf_env(tl_interp *, tl_object *, void *);
void tl_cf_setenv(tl_interp *, tl_object *, void *);
void tl_cf_topenv(tl_interp *, tl_object *, void *);
void tl_cf_type(tl_interp *, tl_object *, void *);

void tl_cf_cons(tl_interp *, tl_object *, void *);
void tl_cf_car(tl_interp *, tl_object *, void *);
void tl_cf_cdr(tl_interp *, tl_object *, void *);
void tl_cf_null(tl_interp *, tl_object *, void *);
void tl_cf_if(tl_interp *, tl_object *, void *);

void tl_cf_add(tl_interp *, tl_object *, void *);
void tl_cf_sub(tl_interp *, tl_object *, void *);
void tl_cf_mul(tl_interp *, tl_object *, void *);
void tl_cf_div(tl_interp *, tl_object *, void *);
void tl_cf_mod(tl_interp *, tl_object *, void *);

void tl_cf_eq(tl_interp *, tl_object *, void *);
void tl_cf_less(tl_interp *, tl_object *, void *);
void tl_cf_nand(tl_interp *, tl_object *, void *);

tl_object *tl_print(tl_interp *, tl_object *);

#define tl_values_push(in, v) (in)->values = tl_new_pair((in), (v), (in)->values)
#define tl_values_pop_into(in, var) do { \
	var = tl_first((in)->values); \
	(in)->values = tl_next((in)->values); \
} while(0)
#define tl_cfunc_return(in, v) do { tl_values_push((in), (v)); return; } while(0)
int tl_push_eval(tl_interp *, tl_object *, tl_object *);
#define TL_APPLY_PUSH_ONLY -1
#define TL_APPLY_INDIRECT -2
void tl_push_apply(tl_interp *, long, tl_object *, tl_object *);
int tl_apply_next(tl_interp *);
void _tl_eval_and_then(tl_interp *, tl_object *, void *, void (*)(tl_interp *, tl_object *, void *), const char *);
#define tl_eval_and_then(in, ex, st, cb) _tl_eval_and_then((in), (ex), (st), (cb), "tl_eval_and_then:" #cb)
void _tl_eval_all_args(tl_interp *, tl_object *, void *, void (*)(tl_interp *, tl_object *, void *), const char *);
#define tl_eval_all_args(in, args, state, cb) _tl_eval_all_args((in), (args), (state), (cb), "tl_eval_all_args:" #cb)
#define tl_run_until_done(in) while(tl_apply_next((in)))

void tl_cf_evalin(tl_interp *, tl_object *, void *);
void tl_cf_call_with_current_continuation(tl_interp *, tl_object *, void *);
/* tl_object *tl_cf_apply(tl_interp *, tl_object *); */

tl_object *tl_read(tl_interp *, tl_object *);

void tl_cf_read(tl_interp *, tl_object *, void *);
void tl_cf_gc(tl_interp *, tl_object *, void *);

#ifdef DEBUG
void tl_dbg_print(tl_object *, int);
void tl_cf_debug_print(tl_interp *, tl_object *, void *);
#endif

#endif
