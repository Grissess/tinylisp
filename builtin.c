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
	in->env = tl_env_set_local(in, in->env, key, tl_first(result));
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
		if(tl_next(l_arg)) tl_putc(in, in->disp_sep);
	}
	tl_printf(in, "\n");
	tl_cfunc_return(in, in->true_);
}

TL_CFBV(display_sep, "display-sep") {
	tl_object *arg;
	if(!args) {
		tl_cfunc_return(in, tl_new_sym_data(in, &in->disp_sep, 1));
	}
	arg = tl_first(args);
	if(!tl_is_sym(arg)) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "tl-display-sep with non-sym"), arg));
		tl_cfunc_return(in, in->false_);
	}
	if(!arg->nm->here.len) {
		tl_error_set(in, tl_new_sym(in, "tl-display-sep with empty sym"));
		tl_cfunc_return(in, in->false_);
	}
	in->disp_sep = arg->nm->here.data[0];
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
	/* DON'T return here--it will offset the stack.
	 * (I thought I understood my code better, but I don't. Consider this free
	 * advice, even thouh I can't explain why.)
	 */
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
	in->env = tl_env_set_global(in, in->env, key, tl_first(result));
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
  tl_object *ret;

	for(tl_list_iter(args, val)) {
		if(tl_is_int(val)) {
			char *buf;
			int sz;
			tl_object *sm;

			sz = snprintf(NULL, 0, "%ld", val->ival);
			assert(sz > 0);
			buf = tl_alloc_malloc(in, sz + 1);
			assert(buf);
			snprintf(buf, sz + 1, "%ld", val->ival);
			val = tl_new_sym(in, buf);
      free(buf);
			l_val->first = val;
		}
		if(tl_is_sym(val)) {
			sz += val->nm->here.len;
		} else {
			tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "concat on non-sym or int"), val));
			tl_cfunc_return(in, in->false_);
		}
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
  ret = tl_new_sym_data(in, buffer, rsz);
  free(buffer);
	tl_cfunc_return(in, ret);
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

TL_CFBV(substr, "substr") {
	tl_object *sym, *start;
	long sidx, eidx;
	char *buf;

	arity_n(in, args, 2, "substr");
	sym = tl_first(args);
	verify_type(in, sym, sym, "substr");
	start = tl_first(tl_next(args));
	verify_type(in, start, int, "substr");
	sidx = start->ival;
	if(tl_next(tl_next(args))) {
		start = tl_first(tl_next(tl_next(args)));
		verify_type(in, start, int, "substr");
		eidx = start->ival;
	} else {
		eidx = sym->nm->here.len;
	}
	if(sidx < 0) {
		sidx += sym->nm->here.len;
		if(sidx < 0) {
			sidx = 0;
		}
	}
	if(eidx < 0) {
		eidx += sym->nm->here.len;
		if(eidx < 0) {
			eidx = 0;
		}
	}
	/* Only test these after testing for negatives; otherwise the integer
	 * promotion makes these unsigned comparisons, always true if the long is
	 * negative.
	 */
	if(sidx >= sym->nm->here.len) {
		sidx = sym->nm->here.len - 1;
	}
	if(eidx > sym->nm->here.len) {
		eidx = sym->nm->here.len;
	}
	if(sidx >= eidx) {
		sidx = eidx;
	}
	tl_cfunc_return(in, tl_new_sym_data(in, sym->nm->here.data + sidx, eidx - sidx));
}

static void _tl_readc_k(tl_interp *in, tl_object *args, tl_object *state) {
	/* A bit superfluous, but we need to name this trampoline anyway, so... */
	tl_cfunc_return(in, tl_first(args));
}

TL_CFBV(readc, "readc") {
	tl_getc_and_then(in, TL_EMPTY_LIST, _tl_readc_k);
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
	tl_object *cur;
	size_t len;

	arity_n(in, args, 2, "apply");
	cur = tl_first(tl_next(args));
	for(len = 0; cur; cur = tl_next(cur), len++) {
		tl_values_push(in, tl_first(cur));
	}
	tl_values_push(in, tl_first(args));
	tl_push_apply(in, TL_APPLY_INDIRECT, tl_new_int(in, len), in->env);
}

TL_CFBV(gc, "gc") {
	tl_gc(in);
	tl_cfunc_return(in, in->true_);
}

TL_CFBV(read, "read") {
	tl_read(in);  /* Returns into the same stack */
}

TL_CFBV(load_mod, "load-mod") {
#ifdef CONFIG_MODULES
	tl_object *name = tl_first(args);
	char *name_cstr = tl_sym_to_cstr(in, name);
	tl_object *ret;

	if(!name_cstr) {
		tl_cfunc_return(in, in->false_);
	}
	ret = _boolify(in->modloadf(in, name_cstr));
	tl_alloc_free(in, name_cstr);
	tl_cfunc_return(in, ret);
#else
	tl_cfunc_return(in, in->false_);
#endif
}

TL_CFBV(rescue, "rescue") {
	tl_object *func, *cont;

	arity_1(in, args, "rescue");
	func = tl_first(args);
	verify_type(in, func, callable, "rescue");

	cont = tl_new_cont(in, in->env, in->conts, in->values);
	tl_rescue_push(in, cont);
	tl_push_apply(in, TL_APPLY_DROP_RESCUE, TL_EMPTY_LIST, TL_EMPTY_LIST);
	tl_push_apply(in, 0, func, in->env);
}

TL_CFBV(all_objects, "all-objects") {
	/* Order is important: capture the top_alloc before making a new object */
	tl_object *cur = in->top_alloc;
	tl_object *res = TL_EMPTY_LIST;
	/* FIXME: if memory pressure causes a GC, this could break spectacularly */
	while(cur) {
		res = tl_new_pair(in, cur, res);
		cur = tl_next_alloc(cur);
	}
	tl_cfunc_return(in, res);
}

#ifdef HAS_MEMINFO

TL_CFBV(meminfo, "meminfo") {
	struct meminfo minfo;

	arity_1(in, args, "meminfo");
	tl_object *idx = tl_first(args);
	verify_type(in, idx, int, "meminfo");

	if(meminfo((size_t) idx->ival, &minfo)) {
		tl_cfunc_return(in, tl_new_pair(in, tl_new_int(in, minfo.size), tl_new_pair(in, tl_new_int(in, minfo.used), tl_new_pair(in, tl_new_int(in, minfo.allocated), TL_EMPTY_LIST))));
	} else {
		tl_cfunc_return(in, in->false_);
	}
}

#endif
