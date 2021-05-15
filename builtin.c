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
		return obj->nm->here.len > 0;
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

TL_CFBV(error, "error") {
	if(args) {
		tl_error_set(in, tl_first(args));
		tl_cfunc_return(in, in->true_);
	} else {
		tl_cfunc_return(in, in->error);
	}
}

TL_CF(macro, "macro") {
	tl_object *fargs = tl_first(args);
	tl_object *envn = tl_first(tl_next(args));
	tl_object *body = tl_next(tl_next(args));
	if(!tl_is_sym(envn)) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "bad macro envname"), envn));
		tl_cfunc_return(in, in->false_);
	}
	tl_cfunc_return(in, tl_new_macro(in, fargs, envn, body, in->env));
}

TL_CF(lambda, "lambda") {
	tl_object *fargs = tl_first(args);
	tl_object *body = tl_next(args);
	tl_cfunc_return(in, tl_new_func(in, fargs, body, in->env));
}

static void _tl_cf_define_k(tl_interp *in, tl_object *result, tl_object *key) {
	tl_env_set_local(in, in->env, key, tl_first(result));
	tl_cfunc_return(in, in->true_);
}

TL_CF(define, "define") {
	tl_object *key = tl_first(args), *val = tl_first(tl_next(args));
	arity_n(in, args, 2, "define");
	if(!tl_is_sym(key)) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "define non-sym"), key));
		tl_cfunc_return(in, in->false_);
	}
	tl_eval_and_then(in, val, key, _tl_cf_define_k);
}

TL_CFBV(display, "display") {
	for(tl_list_iter(args, arg)) {
		tl_print(in, arg);
		if(tl_next(l_arg)) tl_printf(in, "\t");
	}
	tl_printf(in, "\n");
	tl_cfunc_return(in, in->true_);
}

TL_CF(prefix, "prefix") {
	tl_object *prefix = tl_first(args);
	tl_object *name = tl_first(tl_next(args));
	in->prefixes = tl_new_pair(in, tl_new_pair(in, prefix, name), in->prefixes);
	tl_cfunc_return(in, in->true_);
}

TL_CFBV(evalin, "eval-in&") {
	arity_n(in, args, 3, "eval-in&");
	tl_object *env = tl_first(args);
	tl_object *expr = tl_first(tl_next(args));
	tl_object *k = tl_first(tl_next(tl_next(args)));
	tl_push_apply(in, 1, k, in->env);
	tl_push_eval(in, expr, env);
}

TL_CFBV(call_with_current_continuation, "call-with-current-continuation") {
	arity_1(in, args, "call-with-current-continuation");
	tl_object *cont = tl_new_cont(in, in->env, in->conts, in->values);
	tl_push_apply(in, 1, tl_first(args), in->env);
	tl_values_push(in, cont);
}

TL_CFBV(cons, "cons") {
	arity_n(in, args, 2, "cons");
	tl_cfunc_return(in, tl_new_pair(in, tl_first(args), tl_first(tl_next(args))));
}

TL_CFBV(car, "car") {
	arity_1(in, args, "car");
	tl_cfunc_return(in, tl_first(tl_first(args)));
}

TL_CFBV(cdr, "cdr") {
	arity_1(in, args, "cdr");
	tl_cfunc_return(in, tl_next(tl_first(args)));
}

TL_CFBV(null, "null?") {
	arity_1(in, args, "null?");
	tl_cfunc_return(in,  _boolify(!tl_first(args)));
}

static void _tl_cf_if_k(tl_interp *in, tl_object *result, tl_object *branches) {
	tl_object *ift = tl_first(branches), *iff = tl_first(tl_next(branches));
	if(_unboolify(in, tl_first(result))) {
		tl_push_eval(in, ift, in->env);
	} else {
		tl_push_eval(in, iff, in->env);
	}
}

TL_CF(if, "if") {
	tl_object *cond = tl_first(args);
	arity_n(in, args, 3, "if");
	tl_eval_and_then(in, cond, tl_next(args), _tl_cf_if_k);
}

static void _tl_cf_set_k(tl_interp *in, tl_object *result, tl_object *key) {
	tl_env_set_global(in, in->env, key, tl_first(result));
	tl_cfunc_return(in, in->true_);
}

TL_CF(set, "set!") {
	tl_object *key = tl_first(args), *val = tl_first(tl_next(args));
	arity_n(in, args, 2, "set");
	verify_type(in, key, sym, "set!");
	tl_eval_and_then(in, val, key, _tl_cf_set_k);
}

TL_CFBV(env, "env") {
	tl_object *f = tl_first(args);
	if(!f) {
		tl_cfunc_return(in, in->env);
	}
	if(!(tl_is_macro(f) || tl_is_func(f))) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "env of non-func or -macro"), f));
		tl_cfunc_return(in, in->false_);
	}
	tl_cfunc_return(in, f->env);
}

TL_CFBV(setenv, "set-env!") {
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

TL_CFBV(topenv, "top-env") {
	tl_cfunc_return(in, in->top_env);
}

TL_CFBV(concat, "concat") {
	char *buffer, *end, *src;
	size_t sz = 0, rsz;
	for(tl_list_iter(args, val)) {
		verify_type(in, val, sym, "concat");
		sz += val->nm->here.len;
	}
	rsz = sz;
	end = buffer = tl_alloc_malloc(in, sz);
	if(!buffer) {
		tl_error_set(in, tl_new_sym(in, "out of memory"));
		return;
	}
	for(tl_list_iter(args, val)) {
		src = val->nm->here.data;
		sz = val->nm->here.len;
		while(sz > 0) {
			*end++ = *src++;
			sz--;
		}
	}
	tl_cfunc_return(in, tl_new_sym_data(in, buffer, rsz));
}

TL_CFBV(length, "length") {
	arity_1(in, args, "length");
	verify_type(in, tl_first(args), sym, "length");
	tl_cfunc_return(in, tl_new_int(in, tl_first(args)->nm->here.len));
}

TL_CFBV(ord, "ord") {
	arity_n(in, args, 2, "ord");
	verify_type(in, tl_first(args), sym, "ord");
	verify_type(in, tl_first(tl_next(args)), int, "ord");
	if(tl_first(tl_next(args))->ival >= tl_first(args)->nm->here.len) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "ord index out of range"), tl_new_pair(in, tl_first(tl_next(args)), tl_new_int(in, tl_first(args)->nm->here.len))));
		return;
	}
	tl_cfunc_return(in, tl_new_int(in, tl_first(args)->nm->here.data[tl_first(tl_next(args))->ival]));
}

TL_CFBV(chr, "chr") {
	char s[2] = {};
	arity_1(in, args, "chr");
	verify_type(in, tl_first(args), int, "chr");
	s[0] = (char) tl_first(args)->ival;
	tl_cfunc_return(in, tl_new_sym(in, s));
}

TL_CFBV(readc, "readc") {
	tl_cfunc_return(in, tl_new_int(in, tl_getc(in)));
}

TL_CFBV(putbackc, "putbackc") {
	arity_1(in, args, "putback");
	verify_type(in, tl_first(args), int, "putback");
	tl_putback(in, tl_first(args)->ival);
	tl_cfunc_return(in, in->true_);
}

TL_CFBV(writec, "writec") {
	arity_1(in, args, "write");
	verify_type(in, tl_first(args), int, "write");
	tl_putc(in, tl_first(args)->ival);
	tl_cfunc_return(in, in->true_);
}

TL_CFBV(add, "+") {
	long res = 0;
	for(tl_list_iter(args, val)) {
		verify_type(in, val, int, "+");
		res += val->ival;
	}
	tl_cfunc_return(in, tl_new_int(in, res));
}

TL_CFBV(sub, "-") {
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

TL_CFBV(mul, "*") {
	long res = 1;
	for(tl_list_iter(args, val)) {
		verify_type(in, val, int, "*");
		res *= val->ival;
	}
	tl_cfunc_return(in, tl_new_int(in, res));
}

TL_CFBV(div, "/") {
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

TL_CFBV(mod, "%") {
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

TL_CFBV(eq, "=") {
	tl_object *a = tl_first(args), *b = tl_first(tl_next(args));
	if(tl_is_int(a) && tl_is_int(b)) {
		tl_cfunc_return(in, _boolify(a->ival == b->ival));
	}
	if(tl_is_sym(a) && tl_is_sym(b)) {
		tl_cfunc_return(in, _boolify(tl_sym_eq(a, b)));
	}
	tl_cfunc_return(in, _boolify(a == b));
}

TL_CFBV(less, "<") {
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

TL_CFBV(nand, "nand") {
	int a = _unboolify(in, tl_first(args)), b = _unboolify(in, tl_first(tl_next(args)));
	tl_cfunc_return(in, _boolify(!(a && b)));
}

TL_CFBV(type, "type") {
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

TL_CFBV(apply, "apply") {
	arity_1(in, args, "apply");
	tl_values_push(in, tl_first(args));
	tl_push_apply(in, TL_APPLY_INDIRECT, tl_new_int(in, tl_list_len(tl_next(args))), in->env);
}

TL_CFBV(gc, "gc") {
	tl_gc(in);
	tl_cfunc_return(in, in->true_);
}

TL_CFBV(read, "read") {
	tl_cfunc_return(in, tl_read(in, TL_EMPTY_LIST));
}

TL_CFBV(load_mod, "load-mod") {
#ifdef CONFIG_MODULES
	tl_object *name = tl_first(args);
	char *name_cstr;
	tl_object *ret;

	if(!tl_is_sym(name)) {
		tl_cfunc_return(in, in->false_);
	}
	name_cstr = tl_alloc_malloc(in, name->nm->here.len + 1);
	assert(name_cstr);
	memcpy(name_cstr, name->nm->here.data, name->nm->here.len);
	name_cstr[name->nm->here.len] = 0;
	ret = _boolify(in->modloadf(in, name_cstr));
	free(name_cstr);
	tl_cfunc_return(in, ret);
#else
	tl_cfunc_return(in, in->false_);
#endif
}
