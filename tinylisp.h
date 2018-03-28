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
		TL_FUNC,
		TL_MACRO,
	} kind;
	union {
		long ival;
		char *str;
		struct {
			struct tl_object_s *first;
			struct tl_object_s *next;
		};
		struct tl_object_s *(*cfunc)(tl_interp *, struct tl_object_s *);
		struct {
			struct tl_object_s *args;
			struct tl_object_s *body;
			struct tl_object_s *env;
			char *envn;  /* For macros only */
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
tl_object *tl_new_cfunc(tl_interp *, tl_object *(*)(tl_interp *, tl_object *));
tl_object *tl_new_func(tl_interp *, tl_object *, tl_object *, tl_object *);
tl_object *tl_new_macro(tl_interp *, tl_object *, const char *, tl_object *, tl_object *);
void tl_free(tl_interp *, tl_object *);
void tl_gc(tl_interp *);

#define tl_is_int(obj) ((obj) && (obj)->kind == TL_INT)
#define tl_is_sym(obj) ((obj) && (obj)->kind == TL_SYM)
/* FIXME: NULL is a valid empty list */
#define tl_is_pair(obj) (!(obj) || (obj)->kind == TL_PAIR)
#define tl_is_cfunc(obj) ((obj) && (obj)->kind == TL_CFUNC)
#define tl_is_func(obj) ((obj) && (obj)->kind == TL_FUNC)
#define tl_is_macro(obj) ((obj) && (obj)->kind == TL_MACRO)

#define tl_first(obj) ((obj) && tl_is_pair(obj) ? (obj)->first : NULL)
#define tl_next(obj) ((obj) && tl_is_pair(obj) ? (obj)->next : NULL)
#define tl_car tl_first
#define tl_cdr tl_next

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

tl_object *tl_cf_error(tl_interp *, tl_object *);
tl_object *tl_cf_lambda(tl_interp *, tl_object *);
tl_object *tl_cf_macro(tl_interp *, tl_object *);
tl_object *tl_cf_prefix(tl_interp *, tl_object *);
tl_object *tl_cf_define(tl_interp *, tl_object *);
tl_object *tl_cf_set(tl_interp *, tl_object *);
tl_object *tl_cf_env(tl_interp *, tl_object *);
tl_object *tl_cf_setenv(tl_interp *, tl_object *);
tl_object *tl_cf_topenv(tl_interp *, tl_object *);
tl_object *tl_cf_display(tl_interp *, tl_object *);
tl_object *tl_cf_if(tl_interp *, tl_object *);

tl_object *tl_cf_add(tl_interp *, tl_object *);
tl_object *tl_cf_sub(tl_interp *, tl_object *);
tl_object *tl_cf_mul(tl_interp *, tl_object *);
tl_object *tl_cf_div(tl_interp *, tl_object *);
tl_object *tl_cf_mod(tl_interp *, tl_object *);

tl_object *tl_cf_eq(tl_interp *, tl_object *);
tl_object *tl_cf_less(tl_interp *, tl_object *);

tl_object *tl_cf_nand(tl_interp *, tl_object *);

tl_object *tl_cf_cons(tl_interp *, tl_object *);
tl_object *tl_cf_car(tl_interp *, tl_object *);
tl_object *tl_cf_cdr(tl_interp *, tl_object *);
tl_object *tl_cf_type(tl_interp *, tl_object *);
tl_object *tl_cf_null(tl_interp *, tl_object *);

tl_object *tl_print(tl_interp *, tl_object *);

tl_object *tl_eval(tl_interp *, tl_object *);
tl_object *tl_apply(tl_interp *, tl_object *);

tl_object *tl_cf_evalin(tl_interp *, tl_object *);
tl_object *tl_cf_apply(tl_interp *, tl_object *);

tl_object *tl_read(tl_interp *, tl_object *);
tl_object *tl_cf_gc(tl_interp *, tl_object *);

#ifdef DEBUG
void tl_dbg_print(tl_object *, int);
tl_object *tl_cf_debug_print(tl_interp *, tl_object *);
#endif

#endif
