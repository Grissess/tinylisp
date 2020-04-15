#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "tinylisp.h"

#define _boolify(c) ((c) ? in->true_ : in->false_)

int _unboolify(tl_interp *in, tl_object *obj) {
	if(!obj) return 0;
	if(obj == in->false_) return 0;
	if(tl_is_int(obj)) {
		return obj->ival;
	}
	if(tl_is_sym(obj)) {
		return obj->len > 0;
	}
	return 1;
}

#define bad_arity(in, fname) { tl_error_set((in), tl_new_sym((in), "bad " fname " arity")); return; }
#define arity_1(in, args, fname) do { \
	if(!(args)) bad_arity(in, fname) \
} while(0)
#define arity_n(in, args, n, fname) do { \
	if(tl_list_len((args)) < (n)) bad_arity(in, fname) \
} while(0)

#define verify_type(in, obj, type, fname) do { \
	if(!tl_is_##type((obj))) { \
		tl_error_set((in), tl_new_pair((in), tl_new_sym(in, fname " on non-" #type), (obj))); \
		tl_cfunc_return((in), (in)->false_); \
	} \
} while(0)

void tl_cfbv_error(tl_interp *in, tl_object *args, tl_object *_) {
	if(args) {
		tl_error_set(in, tl_first(args));
		tl_cfunc_return(in, in->true_);
	} else {
		tl_cfunc_return(in, in->error);
	}
}

void tl_cf_macro(tl_interp *in, tl_object *args, tl_object *_) {
	tl_object *fargs = tl_first(args);
	tl_object *envn = tl_first(tl_next(args));
	tl_object *body = tl_next(tl_next(args));
	if(!tl_is_sym(envn)) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "bad macro envname"), envn));
		tl_cfunc_return(in, in->false_);
	}
	tl_cfunc_return(in, tl_new_macro(in, fargs, envn, body, in->env));
}

void tl_cf_lambda(tl_interp *in, tl_object *args, tl_object *_) {
	tl_object *fargs = tl_first(args);
	tl_object *body = tl_next(args);
	tl_cfunc_return(in, tl_new_func(in, fargs, body, in->env));
}

void _tl_cf_define_k(tl_interp *in, tl_object *result, tl_object *key) {
	tl_env_set_local(in, in->env, key, tl_first(result));
	tl_cfunc_return(in, in->true_);
}

void tl_cf_define(tl_interp *in, tl_object *args, tl_object *_) {
	tl_object *key = tl_first(args), *val = tl_first(tl_next(args));
	arity_n(in, args, 2, "define");
	if(!tl_is_sym(key)) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "define non-sym"), key));
		tl_cfunc_return(in, in->false_);
	}
	tl_eval_and_then(in, val, key, _tl_cf_define_k);
}

void tl_cfbv_display(tl_interp *in, tl_object *args, tl_object *_) {
	for(tl_list_iter(args, arg)) {
		tl_print(in, arg);
		if(tl_next(l_arg)) tl_printf(in, "\t");
	}
	tl_printf(in, "\n");
	tl_cfunc_return(in, in->true_);
}

void tl_cf_prefix(tl_interp *in, tl_object *args, tl_object *_) {
	tl_object *prefix = tl_first(args);
	tl_object *name = tl_first(tl_next(args));
	in->prefixes = tl_new_pair(in, tl_new_pair(in, prefix, name), in->prefixes);
	tl_cfunc_return(in, in->true_);
}

void tl_cfbv_evalin(tl_interp *in, tl_object *args, tl_object *_) {
	arity_n(in, args, 3, "eval-in&");
	tl_object *env = tl_first(args);
	tl_object *expr = tl_first(tl_next(args));
	tl_object *k = tl_first(tl_next(tl_next(args)));
	tl_push_apply(in, 1, k, in->env);
	tl_push_eval(in, expr, env);
}

void tl_cfbv_call_with_current_continuation(tl_interp *in, tl_object *args, tl_object *_) {
	arity_1(in, args, "call-with-current-continuation");
	tl_object *cont = tl_new_cont(in, in->env, in->conts, in->values);
	tl_push_apply(in, 1, tl_first(args), in->env);
	tl_values_push(in, cont);
}

void tl_cfbv_cons(tl_interp *in, tl_object *args, tl_object *_) {
	arity_n(in, args, 2, "cons");
	tl_cfunc_return(in, tl_new_pair(in, tl_first(args), tl_first(tl_next(args))));
}

void tl_cfbv_car(tl_interp *in, tl_object *args, tl_object *_) {
	arity_1(in, args, "car");
	tl_cfunc_return(in, tl_first(tl_first(args)));
}

void tl_cfbv_cdr(tl_interp *in, tl_object *args, tl_object *_) {
	arity_1(in, args, "cdr");
	tl_cfunc_return(in, tl_next(tl_first(args)));
}

void tl_cfbv_null(tl_interp *in, tl_object *args, tl_object *_) {
	arity_1(in, args, "null?");
	tl_cfunc_return(in,  _boolify(!tl_first(args)));
}

void _tl_cf_if_k(tl_interp *in, tl_object *result, tl_object *branches) {
	tl_object *ift = tl_first(branches), *iff = tl_first(tl_next(branches));
	if(_unboolify(in, tl_first(result))) {
		tl_push_eval(in, ift, in->env);
	} else {
		tl_push_eval(in, iff, in->env);
	}
}

void tl_cf_if(tl_interp *in, tl_object *args, tl_object *_) {
	tl_object *cond = tl_first(args);
	arity_n(in, args, 3, "if");
	tl_eval_and_then(in, cond, tl_next(args), _tl_cf_if_k);
}

void _tl_cf_set_k(tl_interp *in, tl_object *result, tl_object *key) {
	tl_env_set_global(in, in->env, key, tl_first(result));
	tl_cfunc_return(in, in->true_);
}

void tl_cf_set(tl_interp *in, tl_object *args, tl_object *_) {
	tl_object *key = tl_first(args), *val = tl_first(tl_next(args));
	arity_n(in, args, 2, "set");
	verify_type(in, key, sym, "set!");
	tl_eval_and_then(in, val, key, _tl_cf_set_k);
}

void tl_cfbv_env(tl_interp *in, tl_object *result, tl_object *_) {
	tl_object *f = tl_first(result);
	if(!f) {
		tl_cfunc_return(in, in->env);
	}
	if(!(tl_is_macro(f) || tl_is_func(f))) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "env of non-func or -macro"), f));
		tl_cfunc_return(in, in->false_);
	}
	tl_cfunc_return(in, f->env);
}

void tl_cfbv_setenv(tl_interp *in, tl_object *args, tl_object *_) {
	tl_object *first = tl_first(args), *next = tl_first(tl_next(args));
	if(!next) {
		in->env = first;
		tl_cfunc_return(in, in->true_);
	}
	if(!(tl_is_macro(first) || tl_is_func(first))) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "setenv on non-func or -macro"), first));
		tl_cfunc_return(in, in->false_);
	}
	first->env = next;
	tl_cfunc_return(in, in->true_);
}

void tl_cfbv_topenv(tl_interp *in, tl_object *args, tl_object *_) {
	tl_cfunc_return(in, in->top_env);
}

void tl_cfbv_concat(tl_interp *in, tl_object *args, tl_object *_) {
	char *buffer, *end, *src;
	size_t sz = 0, rsz;
	for(tl_list_iter(args, val)) {
		verify_type(in, val, sym, "concat");
		sz += val->len;
	}
	rsz = sz;
	end = buffer = tl_alloc_malloc(in, sz);
	if(!buffer) tl_error_set(in, tl_new_sym(in, "out of memory"));
	for(tl_list_iter(args, val)) {
		src = val->str;
		sz = val->len;
		while(sz > 0) {
			*end++ = *src++;
			sz--;
		}
	}
	tl_cfunc_return(in, tl_new_sym_data(in, buffer, rsz));
}

void tl_cfbv_length(tl_interp *in, tl_object *args, tl_object *_) {
	arity_1(in, args, "length");
	verify_type(in, tl_first(args), sym, "length");
	tl_cfunc_return(in, tl_new_int(in, tl_first(args)->len));
}

void tl_cfbv_ord(tl_interp *in, tl_object *args, tl_object *_) {
	arity_n(in, args, 2, "ord");
	verify_type(in, tl_first(args), sym, "ord");
	verify_type(in, tl_first(tl_next(args)), int, "ord");
	tl_cfunc_return(in, tl_new_int(in, tl_first(args)->str[tl_first(tl_next(args))->ival]));
}

void tl_cfbv_chr(tl_interp *in, tl_object *args, tl_object *_) {
	char s[2] = {};
	arity_1(in, args, "chr");
	verify_type(in, tl_first(args), int, "chr");
	s[0] = (char) tl_first(args)->ival;
	tl_cfunc_return(in, tl_new_sym(in, s));
}

void tl_cfbv_readc(tl_interp *in, tl_object *args, tl_object *_) {
	tl_cfunc_return(in, tl_new_int(in, tl_getc(in)));
}

void tl_cfbv_putbackc(tl_interp *in, tl_object *args, tl_object *_) {
	arity_1(in, args, "putback");
	verify_type(in, tl_first(args), int, "putback");
	tl_putback(in, tl_first(args)->ival);
	tl_cfunc_return(in, in->true_);
}

void tl_cfbv_writec(tl_interp *in, tl_object *args, tl_object *_) {
	arity_1(in, args, "write");
	verify_type(in, tl_first(args), int, "write");
	tl_putc(in, tl_first(args)->ival);
	tl_cfunc_return(in, in->true_);
}

void tl_cfbv_add(tl_interp *in, tl_object *args, tl_object *_) {
	long res = 0;
	for(tl_list_iter(args, val)) {
		verify_type(in, val, int, "+");
		res += val->ival;
	}
	tl_cfunc_return(in, tl_new_int(in, res));
}

void tl_cfbv_sub(tl_interp *in, tl_object *args, tl_object *_) {
	long phase = 0;
	long res = 0;
	for(tl_list_iter(args, val)) {
		verify_type(in, val, int, "-");
		if(!phase) {
			res += val->ival;
			phase = 1;
		} else {
			res -= val->ival;
		}
	}
	tl_cfunc_return(in, tl_new_int(in, res));
}

void tl_cfbv_mul(tl_interp *in, tl_object *args, tl_object *_) {
	long res = 1;
	for(tl_list_iter(args, val)) {
		verify_type(in, val, int, "*");
		res *= val->ival;
	}
	tl_cfunc_return(in, tl_new_int(in, res));
}

void tl_cfbv_div(tl_interp *in, tl_object *args, tl_object *_) {
	long phase = 0;
	long res = 1;
	for(tl_list_iter(args, val)) {
		verify_type(in, val, int, "/");
		if(!phase) {
			res *= val->ival;
			phase = 1;
		} else {
			res /= val->ival;
		}
	}
	tl_cfunc_return(in, tl_new_int(in, res));
}

void tl_cfbv_mod(tl_interp *in, tl_object *args, tl_object *_) {
	long phase = 0;
	long res = 1;
	for(tl_list_iter(args, val)) {
		verify_type(in, val, int, "%");
		if(!phase) {
			res *= val->ival;
			phase = 1;
		} else {
			res %= val->ival;
		}
	}
	tl_cfunc_return(in, tl_new_int(in, res));
}

void tl_cfbv_eq(tl_interp *in, tl_object *args, tl_object *_) {
	tl_object *a = tl_first(args), *b = tl_first(tl_next(args));
	if(tl_is_int(a) && tl_is_int(b)) {
		tl_cfunc_return(in, _boolify(a->ival == b->ival));
	}
	if(tl_is_sym(a) && tl_is_sym(b)) {
		tl_cfunc_return(in, _boolify(tl_sym_eq(a, b)));
	}
	tl_cfunc_return(in, _boolify(a == b));
}

void tl_cfbv_less(tl_interp *in, tl_object *args, tl_object *_) {
	tl_object *a = tl_first(args), *b = tl_first(tl_next(args));
	if(tl_is_int(a) && tl_is_int(b)) {
		tl_cfunc_return(in, _boolify(a->ival < b->ival));
	}
	if(tl_is_sym(a) && tl_is_sym(b)) {
		tl_cfunc_return(in, _boolify(tl_sym_less(a, b)));
	}
	tl_error_set(in, tl_new_pair(in, tl_new_pair(in, tl_new_sym(in, "incomparable types"), a), b));
	tl_cfunc_return(in, in->false_);
}

void tl_cfbv_nand(tl_interp *in, tl_object *args, tl_object *_) {
	int a = _unboolify(in, tl_first(args)), b = _unboolify(in, tl_first(tl_next(args)));
	tl_cfunc_return(in, _boolify(!(a && b)));
}

void tl_cfbv_type(tl_interp *in, tl_object *args, tl_object *_) {
	tl_object *obj = tl_first(args);
	if(tl_has_error(in)) {
		tl_cfunc_return(in, in->false_);
	}
	/* Check for pairs last, since NULL is a valid pair */
	if(tl_is_int(obj)) tl_cfunc_return(in, tl_new_sym(in, "int"));
	if(tl_is_sym(obj)) tl_cfunc_return(in, tl_new_sym(in, "sym"));
	if(tl_is_cfunc(obj)) tl_cfunc_return(in, tl_new_sym(in, "cfunc"));
	if(tl_is_cfunc_byval(obj)) tl_cfunc_return(in, tl_new_sym(in, "cfunc_byval"));
	if(tl_is_func(obj)) tl_cfunc_return(in, tl_new_sym(in, "func"));
	if(tl_is_macro(obj)) tl_cfunc_return(in, tl_new_sym(in, "macro"));
	if(tl_is_cont(obj)) tl_cfunc_return(in, tl_new_sym(in, "cont"));
	if(tl_is_pair(obj)) tl_cfunc_return(in, tl_new_sym(in, "pair"));
	tl_cfunc_return(in, tl_new_sym(in, "unknown"));
}

#if 0
tl_object *tl_cf_apply(tl_interp *in, tl_object *args) {
	tl_object *list = TL_EMPTY_LIST;
	for(tl_list_iter(args, item)) {
		list = tl_new_pair(in, tl_eval(in, item), list);
	}
	return tl_apply(in, tl_list_rvs(in, list));
}
#endif

void tl_cfbv_gc(tl_interp *in, tl_object *args, tl_object *_) {
	tl_gc(in);
	tl_cfunc_return(in, in->true_);
}

void tl_cfbv_read(tl_interp *in, tl_object *args, tl_object *_) {
	tl_cfunc_return(in, tl_read(in, TL_EMPTY_LIST));
}

#ifdef CONFIG_MODULES
void tl_cfbv_load_mod(tl_interp *in, tl_object *args, tl_object *_) {
	tl_object *name = tl_first(args);
	char *name_cstr;
	tl_object *ret;

	if(!tl_is_sym(name)) {
		tl_cfunc_return(in, in->false_);
	}
	name_cstr = tl_alloc_malloc(in, name->len + 1);
	assert(name_cstr);
	memcpy(name_cstr, name->str, name->len);
	name_cstr[name->len] = 0;
	ret = _boolify(in->modloadf(in, name_cstr));
	free(name_cstr);
	tl_cfunc_return(in, ret);
}
#else
void tl_cfbv_load_mod(tl_interp *in, tl_object *args, tl_object *_) {
	tl_cfunc_return(in, in->false_);
}
#endif
