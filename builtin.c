#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "tinylisp.h"

#define _boolify(c) ((c) ? in->true_ : in->false_)

int _unboolify(tl_interp *in, tl_object *obj) {
	if(!obj) return 0;
	if(obj == in->false_) return 0;
	if(tl_is_int(obj)) {
		return obj->ival;
	}
	if(tl_is_sym(obj)) {
		return strlen(obj->str) > 0;
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

void _tl_cf_error_k(tl_interp *in, tl_object *result, void *_) {
	tl_error_set(in, tl_first(result));
	tl_cfunc_return(in, in->true_);
}

void tl_cf_error(tl_interp *in, tl_object *args, void *_) {
	if(args) {
		tl_eval_and_then(in, tl_first(args), in->env, _tl_cf_error_k);
	} else {
		tl_cfunc_return(in, in->error);
	}
}

void tl_cf_macro(tl_interp *in, tl_object *args, void *_) {
	tl_object *fargs = tl_first(args);
	tl_object *envn = tl_first(tl_next(args));
	tl_object *body = tl_next(tl_next(args));
	if(!tl_is_sym(envn)) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "bad macro envname"), envn));
		tl_cfunc_return(in, in->false_);
	}
	tl_cfunc_return(in, tl_new_macro(in, fargs, envn->str, body, in->env));
}

void tl_cf_lambda(tl_interp *in, tl_object *args, void *_) {
	tl_object *fargs = tl_first(args);
	tl_object *body = tl_next(args);
	tl_cfunc_return(in, tl_new_func(in, fargs, body, in->env));
}

void _tl_cf_define_k(tl_interp *in, tl_object *result, void *nm) {
	tl_env_set_local(in, in->env, nm, tl_first(result));
	tl_cfunc_return(in, in->true_);
}

void tl_cf_define(tl_interp *in, tl_object *args, void *_) {
	tl_object *key = tl_first(args), *val = tl_first(tl_next(args));
	arity_n(in, args, 2, "define");
	if(!tl_is_sym(key)) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "define non-sym"), key));
		tl_cfunc_return(in, in->false_);
	}
	tl_eval_and_then(in, val, key->str, _tl_cf_define_k);
}

void _tl_cf_display_k(tl_interp *in, tl_object *result, void *next) {
	tl_print(in, tl_first(result));
	if(next) {
		in->printf(in->udata, "\t");
		tl_eval_and_then(in, tl_first((tl_object *) next), tl_next((tl_object *) next), _tl_cf_display_k);
	} else {
		in->printf(in->udata, "\n");
		tl_cfunc_return(in, in->true_);
	}
}

void tl_cf_display(tl_interp *in, tl_object *args, void *_) {
	tl_eval_and_then(in, tl_first(args), tl_next(args), _tl_cf_display_k);
}

void tl_cf_prefix(tl_interp *in, tl_object *args, void *_) {
	tl_object *prefix = tl_first(args);
	tl_object *name = tl_first(tl_next(args));
	in->prefixes = tl_new_pair(in, tl_new_pair(in, prefix, name), in->prefixes);
	tl_cfunc_return(in, in->true_);
}

void _tl_cf_evalin_k(tl_interp *in, tl_object *args, void *_) {
	tl_object *env = tl_first(args);
	tl_object *expr = tl_first(tl_next(args));
	tl_object *k = tl_first(tl_next(tl_next(args)));
	tl_push_apply(in, 1, k, in->env);
	tl_push_eval(in, expr, env);
}

void tl_cf_evalin(tl_interp *in, tl_object *args, void *_) {
	arity_n(in, args, 3, "eval-in&");
	tl_eval_all_args(in, args, NULL, _tl_cf_evalin_k);
}

void _tl_cf_call_with_current_continuation_k(tl_interp *in, tl_object *args, void *_) {
	tl_object *cont = tl_new_cont(in, in->env, in->conts, in->values);
	tl_push_apply(in, 1, tl_first(args), in->env);
	tl_values_push(in, cont);
}

void tl_cf_call_with_current_continuation(tl_interp *in, tl_object *args, void *_) {
	arity_1(in, args, "call-with-current-continuation");
	tl_eval_all_args(in, args, NULL, _tl_cf_call_with_current_continuation_k);
}

void _tl_cf_cons_k(tl_interp *in, tl_object *args, void *_) {
	tl_cfunc_return(in, tl_new_pair(in, tl_first(args), tl_first(tl_next(args))));
}

void tl_cf_cons(tl_interp *in, tl_object *args, void *_) {
	arity_1(in, args, "cons");
	tl_eval_all_args(in, args, NULL, _tl_cf_cons_k);
}

void _tl_cf_car_k(tl_interp *in, tl_object *args, void *_) {
	tl_cfunc_return(in, tl_first(tl_first(args)));
}

void tl_cf_car(tl_interp *in, tl_object *args, void *_) {
	arity_1(in, args, "car");
	tl_eval_all_args(in, args, NULL, _tl_cf_car_k);
}

void _tl_cf_cdr_k(tl_interp *in, tl_object *args, void *_) {
	tl_cfunc_return(in, tl_next(tl_first(args)));
}

void tl_cf_cdr(tl_interp *in, tl_object *args, void *_) {
	arity_1(in, args, "cdr");
	tl_eval_all_args(in, args, NULL, _tl_cf_cdr_k);
}

void _tl_cf_null_k(tl_interp *in, tl_object *args, void *_) {
	tl_cfunc_return(in,  _boolify(!tl_first(args)));
}

void tl_cf_null(tl_interp *in, tl_object *args, void *_) {
	arity_1(in, args, "null?");
	tl_eval_all_args(in, args, NULL, _tl_cf_null_k);
}

void _tl_cf_if_k(tl_interp *in, tl_object *result, void *_branches) {
	tl_object *branches = _branches;
	tl_object *ift = tl_first(branches), *iff = tl_first(tl_next(branches));
	if(_unboolify(in, tl_first(result))) {
		tl_push_eval(in, ift, in->env);
	} else {
		tl_push_eval(in, iff, in->env);
	}
}

void tl_cf_if(tl_interp *in, tl_object *args, void *_) {
	tl_object *cond = tl_first(args);
	arity_n(in, args, 3, "if");
	tl_eval_and_then(in, cond, tl_next(args), _tl_cf_if_k);
}

void _tl_cf_set_k(tl_interp *in, tl_object *result, void *nm) {
	tl_env_set_global(in, in->env, nm, tl_first(result));
	tl_cfunc_return(in, in->true_);
}

void tl_cf_set(tl_interp *in, tl_object *args, void *_) {
	tl_object *key = tl_first(args), *val = tl_first(tl_next(args));
	arity_n(in, args, 2, "set");
	verify_type(in, key, sym, "set!");
	tl_eval_and_then(in, val, key->str, _tl_cf_set_k);
}

void _tl_cf_env_k(tl_interp *in, tl_object *result, void *_) {
	tl_object *f = tl_first(result);
	if(!(tl_is_macro(f) || tl_is_func(f))) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "env of non-func or -macro"), f));
		tl_cfunc_return(in, in->false_);
	}
	tl_cfunc_return(in, f->env);
}

void tl_cf_env(tl_interp *in, tl_object *args, void *_) {
	tl_object *f = tl_first(args);
	if(!f) {
		tl_cfunc_return(in, in->env);
	}
	tl_eval_and_then(in, f, in->env, _tl_cf_env_k);
}

void _tl_cf_setenv_global_k(tl_interp *in, tl_object *result, void *_) {
	in->env = tl_first(result);
	tl_cfunc_return(in, in->true_);
}

void _tl_cf_setenv_k(tl_interp *in, tl_object *args, void *_) {
	tl_object *first = tl_first(args), *next = tl_first(tl_next(args));
	if(!(tl_is_macro(first) || tl_is_func(first))) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "setenv on non-func or -macro"), first));
		tl_cfunc_return(in, in->false_);
	}
	first->env = next;
	tl_cfunc_return(in, in->true_);
}

void tl_cf_setenv(tl_interp *in, tl_object *args, void *_) {
	tl_object *first = tl_first(args), *next = tl_first(tl_next(args));
	arity_1(in, args, "set-env!");
	if(!next) {
		tl_eval_and_then(in, first, in->env, _tl_cf_setenv_global_k);
		return;
	}
	tl_eval_all_args(in, args, NULL, _tl_cf_setenv_k);
}

void tl_cf_topenv(tl_interp *in, tl_object *args, void *_) {
	tl_cfunc_return(in, in->top_env);
}

void _tl_cf_concat_k(tl_interp *in, tl_object *args, void *_) {
	char *buffer = "", *new_buffer;
	char *orig_buffer = buffer;
	size_t sz;
	for(tl_list_iter(args, val)) {
		verify_type(in, val, sym, "concat");
		sz = snprintf(NULL, 0, "%s%s", buffer, val->str);
		new_buffer = malloc(sz + 1);
		snprintf(new_buffer, sz + 1, "%s%s", buffer, val->str);
		if(buffer != orig_buffer) free(buffer);
		buffer = new_buffer;
	}
	tl_cfunc_return(in, tl_new_sym(in, buffer));
}

void tl_cf_concat(tl_interp *in, tl_object *args, void *_) {
	tl_eval_all_args(in, args, NULL, _tl_cf_concat_k);
}

void _tl_cf_length_k(tl_interp *in, tl_object *result, void *_) {
	verify_type(in, tl_first(result), sym, "length");
	tl_cfunc_return(in, tl_new_int(in, strlen(tl_first(result)->str)));
}

void tl_cf_length(tl_interp *in, tl_object *args, void *_) {
	tl_eval_and_then(in, tl_first(args), NULL, _tl_cf_length_k);
}

void _tl_cf_add_k(tl_interp *in, tl_object *args, void *_) {
	long res = 0;
	for(tl_list_iter(args, val)) {
		verify_type(in, val, int, "+");
		res += val->ival;
	}
	tl_cfunc_return(in, tl_new_int(in, res));
}

void tl_cf_add(tl_interp *in, tl_object *args, void *_) {
	tl_eval_all_args(in, args, NULL, _tl_cf_add_k);
}

void _tl_cf_sub_k(tl_interp *in, tl_object *args, void *_) {
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

void tl_cf_sub(tl_interp *in, tl_object *args, void *_) {
	tl_eval_all_args(in, args, NULL, _tl_cf_sub_k);
}

void _tl_cf_mul_k(tl_interp *in, tl_object *args, void *_) {
	long res = 1;
	for(tl_list_iter(args, val)) {
		verify_type(in, val, int, "*");
		res *= val->ival;
	}
	tl_cfunc_return(in, tl_new_int(in, res));
}

void tl_cf_mul(tl_interp *in, tl_object *args, void *_) {
	tl_eval_all_args(in, args, NULL, _tl_cf_mul_k);
}

void _tl_cf_div_k(tl_interp *in, tl_object *args, void *_) {
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

void tl_cf_div(tl_interp *in, tl_object *args, void *_) {
	tl_eval_all_args(in, args, NULL, _tl_cf_div_k);
}

void _tl_cf_mod_k(tl_interp *in, tl_object *args, void *_) {
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

void tl_cf_mod(tl_interp *in, tl_object *args, void *_) {
	tl_eval_all_args(in, args, NULL, _tl_cf_mod_k);
}

void _tl_cf_eq_k(tl_interp *in, tl_object *args, void *_) {
	tl_object *a = tl_first(args), *b = tl_first(tl_next(args));
	if(tl_is_int(a) && tl_is_int(b)) {
		tl_cfunc_return(in, _boolify(a->ival == b->ival));
	}
	if(tl_is_sym(a) && tl_is_sym(b)) {
		tl_cfunc_return(in,  _boolify(!strcmp(a->str, b->str)));
	}
	tl_cfunc_return(in, _boolify(a == b));
}

void tl_cf_eq(tl_interp *in, tl_object *args, void *_) {
	tl_eval_all_args(in, args, NULL, _tl_cf_eq_k);
}

void _tl_cf_less_k(tl_interp *in, tl_object *args, void *_) {
	tl_object *a = tl_first(args), *b = tl_first(tl_next(args));
	if(tl_is_int(a) && tl_is_int(b)) {
		tl_cfunc_return(in, _boolify(a->ival < b->ival));
	}
	if(tl_is_sym(a) && tl_is_sym(b)) {
		tl_cfunc_return(in, _boolify(strcmp(a->str, b->str) < 0));
	}
	tl_error_set(in, tl_new_pair(in, tl_new_pair(in, tl_new_sym(in, "incomparable types"), a), b));
	tl_cfunc_return(in, in->false_);
}

void tl_cf_less(tl_interp *in, tl_object *args, void *_) {
	tl_eval_all_args(in, args, NULL, _tl_cf_less_k);
}

void _tl_cf_nand_k(tl_interp *in, tl_object *args, void *_) {
	int a = _unboolify(in, tl_first(args)), b = _unboolify(in, tl_first(tl_next(args)));
	tl_cfunc_return(in, _boolify(!(a && b)));
}

void tl_cf_nand(tl_interp *in, tl_object *args, void *_) {
	tl_eval_all_args(in, args, NULL, _tl_cf_nand_k);
}

void _tl_cf_type_k(tl_interp *in, tl_object *result, void *_) {
	tl_object *obj = tl_first(result);
	if(tl_has_error(in)) {
		tl_cfunc_return(in, in->false_);
	}
	/* Check for pairs last, since NULL is a valid pair */
	if(tl_is_int(obj)) tl_cfunc_return(in, tl_new_sym(in, "int"));
	if(tl_is_sym(obj)) tl_cfunc_return(in, tl_new_sym(in, "sym"));
	if(tl_is_cfunc(obj)) tl_cfunc_return(in, tl_new_sym(in, "cfunc"));
	if(tl_is_func(obj)) tl_cfunc_return(in, tl_new_sym(in, "func"));
	if(tl_is_macro(obj)) tl_cfunc_return(in, tl_new_sym(in, "macro"));
	if(tl_is_cont(obj)) tl_cfunc_return(in, tl_new_sym(in, "cont"));
	if(tl_is_pair(obj)) tl_cfunc_return(in, tl_new_sym(in, "pair"));
	tl_cfunc_return(in, tl_new_sym(in, "unknown"));
}

void tl_cf_type(tl_interp *in, tl_object *args, void *_) {
	tl_eval_and_then(in, tl_first(args), NULL, _tl_cf_type_k);
}

#if 0
tl_object *tl_cf_evalin(tl_interp *in, tl_object *args) {
	tl_object *env = tl_eval(in, tl_first(args));
	tl_object *expr = tl_eval(in, tl_first(tl_next(args)));
	tl_object *old_env = in->env;
	in->env = env;
	tl_object *res = tl_eval(in, expr);
	in->env = old_env;
	return res;
}

tl_object *tl_cf_apply(tl_interp *in, tl_object *args) {
	tl_object *list = TL_EMPTY_LIST;
	for(tl_list_iter(args, item)) {
		list = tl_new_pair(in, tl_eval(in, item), list);
	}
	return tl_apply(in, tl_list_rvs(in, list));
}
#endif

void tl_cf_gc(tl_interp *in, tl_object *args, void *_) {
	tl_gc(in);
	tl_cfunc_return(in, in->true_);
}

void tl_cf_read(tl_interp *in, tl_object *args, void *_) {
	tl_cfunc_return(in, tl_read(in, TL_EMPTY_LIST));
}
