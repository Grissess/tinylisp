#include <string.h>

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

tl_object *tl_cf_error(tl_interp *in, tl_object *args) {
	if(args) {
		tl_error_set(in, tl_eval(in, args));
		return in->true_;
	} else {
		return in->error;
	}
}

tl_object *tl_cf_lambda(tl_interp *in, tl_object *args) {
	tl_object *fargs = tl_first(args);
	tl_object *body = tl_next(args);
	return tl_new_func(in, fargs, body, in->env);
}

tl_object *tl_cf_macro(tl_interp *in, tl_object *args) {
	tl_object *fargs = tl_first(args);
	tl_object *envn = tl_first(tl_next(args));
	tl_object *body = tl_next(tl_next(args));
	if(!tl_is_sym(envn)) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "Bad macro envname"), envn));
		return in->false_;
	}
	return tl_new_macro(in, fargs, envn->str, body, in->env);
}

tl_object *tl_cf_prefix(tl_interp *in, tl_object *args) {
	tl_object *prefix = tl_first(args);
	tl_object *name = tl_first(tl_next(args));
	in->prefixes = tl_new_pair(in, tl_new_pair(in, prefix, name), in->prefixes);
	return in->true_;
}

tl_object *tl_cf_define(tl_interp *in, tl_object *args) {
	tl_object *key = tl_first(args), *val = tl_first(tl_next(args));
	if(!key) {
		tl_error_set(in, tl_new_sym(in, "Bad define arity"));
		return in->false_;
	}
	if(!tl_is_sym(key)) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "Define non-sym"), key));
		return in->false_;
	}
	val = tl_eval(in, val);
	tl_env_set_local(in, in->env, key->str, val);
	return in->true_;
}

tl_object *tl_cf_set(tl_interp *in, tl_object *args) {
	tl_object *key = tl_first(args), *val = tl_first(tl_next(args));
	if(!key) {
		tl_error_set(in, tl_new_sym(in, "Bad define arity"));
		return in->false_;
	}
	if(!tl_is_sym(key)) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "Define non-sym"), key));
		return in->false_;
	}
	val = tl_eval(in, val);
	tl_env_set_global(in, in->env, key->str, val);
	return in->true_;
}

tl_object *tl_cf_env(tl_interp *in, tl_object *args) {
	tl_object *f = tl_first(args);
	if(!f) {
		return in->env;
	}
	f = tl_eval(in, args);
	if(!(tl_is_func(f) || tl_is_macro(f))) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "Env of non-func or -macro"), f));
		return in->false_;
	}
	return f->env;
}

tl_object *tl_cf_setenv(tl_interp *in, tl_object *args) {
	tl_object *first = tl_first(args), *next = tl_first(tl_next(args));
	if(!next) {
		in->env = tl_eval(in, first);
		return in->true_;
	}
	first = tl_eval(in, first);
	if(!(tl_is_func(first) || tl_is_macro(first))) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "Setenv on non-func or -macro"), first));
		return in->false_;
	}
	next = tl_eval(in, next);
	first->env = next;
	return in->true_;
}

tl_object *tl_cf_topenv(tl_interp *in, tl_object *args) {
	return in->top_env;
}

tl_object *tl_cf_display(tl_interp *in, tl_object *args) {
	for(tl_list_iter(args, item)) {
		tl_print(in, tl_eval(in, item));
		in->printf(in->udata, "\t");
	}
	in->printf(in->udata, "\n");
	return in->true_;
}

tl_object *tl_cf_if(tl_interp *in, tl_object *args) {
	tl_object *cond = tl_first(args), *ift = tl_first(tl_next(args)), *iff = tl_first(tl_next(tl_next(args)));
	if(!cond || !ift || !iff) {
		tl_error_set(in, tl_new_sym(in, "Bad if arity"));
		return in->false_;
	}
	cond = tl_eval(in, cond);
	if(_unboolify(in, cond)) {
		return tl_eval(in, ift);
	}
	return tl_eval(in, iff);
}

tl_object *tl_cf_add(tl_interp *in, tl_object *args) {
	long res = 0;
	tl_object *val;
	for(tl_list_iter(args, item)) {
		val = tl_eval(in, item);
		if(!tl_is_int(val)) {
			tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "+ on non-int"), item));
			return in->false_;
		}
		res += val->ival;
	}
	return tl_new_int(in, res);
}

tl_object *tl_cf_sub(tl_interp *in, tl_object *args) {
	long phase = 0;
	long res = 0;
	tl_object *val;
	for(tl_list_iter(args, item)) {
		val = tl_eval(in, item);
		if(!tl_is_int(val)) {
			tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "- on non-int"), item));
			return in->false_;
		}
		if(!phase) {
			res += val->ival;
			phase = 1;
		} else {
			res -= val->ival;
		}
	}
	return tl_new_int(in, res);
}

tl_object *tl_cf_mul(tl_interp *in, tl_object *args) {
	long res = 1;
	tl_object *val;
	for(tl_list_iter(args, item)) {
		val = tl_eval(in, item);
		if(!tl_is_int(val)) {
			tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "* on non-int"), item));
			return in->false_;
		}
		res *= val->ival;
	}
	return tl_new_int(in, res);
}

tl_object *tl_cf_div(tl_interp *in, tl_object *args) {
	long phase = 0;
	long res = 1;
	tl_object *val;
	for(tl_list_iter(args, item)) {
		val = tl_eval(in, item);
		if(!tl_is_int(val)) {
			tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "/ on non-int"), item));
			return in->false_;
		}
		if(!phase) {
			res *= val->ival;
			phase = 1;
		} else {
			res /= val->ival;
		}
	}
	return tl_new_int(in, res);
}

tl_object *tl_cf_mod(tl_interp *in, tl_object *args) {
	long phase = 0;
	long res = 1;
	tl_object *val;
	for(tl_list_iter(args, item)) {
		val = tl_eval(in, item);
		if(!tl_is_int(val)) {
			tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "/ on non-int"), item));
			return in->false_;
		}
		if(!phase) {
			res *= val->ival;
			phase = 1;
		} else {
			res %= val->ival;
		}
	}
	return tl_new_int(in, res);
}

tl_object *tl_cf_eq(tl_interp *in, tl_object *args) {
	tl_object *a = tl_first(args), *b = tl_first(tl_next(args));
	a = tl_eval(in, a);
	b = tl_eval(in, b);
	if(tl_is_int(a) && tl_is_int(b)) {
		return _boolify(a->ival == b->ival);
	}
	if(tl_is_sym(a) && tl_is_sym(b)) {
		return _boolify(!strcmp(a->str, b->str));
	}
	return _boolify(a == b);
}

tl_object *tl_cf_less(tl_interp *in, tl_object *args) {
	tl_object *a = tl_first(args), *b = tl_first(tl_next(args));
	a = tl_eval(in, a);
	b = tl_eval(in, b);
	if(tl_is_int(a) && tl_is_int(b)) {
		return _boolify(a->ival < b->ival);
	}
	if(tl_is_sym(a) && tl_is_sym(b)) {
		return _boolify(strcmp(a->str, b->str) < 0);
	}
	tl_error_set(in, tl_new_pair(in, tl_new_pair(in, tl_new_sym(in, "Unsortable types"), a), b));
	return in->false_;
}

tl_object *tl_cf_nand(tl_interp *in, tl_object *args) {
	int a = _unboolify(in, tl_eval(in, tl_first(args))), b = _unboolify(in, tl_eval(in, tl_first(tl_next(args))));
	return _boolify(!(a && b));
}

tl_object *tl_cf_cons(tl_interp *in, tl_object *args) {
	tl_object *first = tl_eval(in, tl_first(args)), *next = tl_eval(in, tl_first(tl_next(args)));
	return tl_new_pair(in, first, next);
}

tl_object *tl_cf_car(tl_interp *in, tl_object *args) {
	return tl_first(tl_eval(in, tl_first(args)));
}

tl_object *tl_cf_cdr(tl_interp *in, tl_object *args) {
	return tl_next(tl_eval(in, tl_first(args)));
}

tl_object *tl_cf_type(tl_interp *in, tl_object *args) {
	tl_object *obj = tl_eval(in, tl_first(args));
	if(tl_has_error(in)) {
		return in->false_;
	}
	/* Check for pairs last, since NULL is a valid pair */
	if(tl_is_int(obj)) return tl_new_sym(in, "int");
	if(tl_is_sym(obj)) return tl_new_sym(in, "sym");
	if(tl_is_cfunc(obj)) return tl_new_sym(in, "cfunc");
	if(tl_is_func(obj)) return tl_new_sym(in, "func");
	if(tl_is_macro(obj)) return tl_new_sym(in, "macro");
	if(tl_is_pair(obj)) return tl_new_sym(in, "pair");
	return tl_new_sym(in, "unknown");
}

tl_object *tl_cf_null(tl_interp *in, tl_object *args) {
	tl_object *first = tl_first(args);
	if(!first) {
		tl_error_set(in, tl_new_sym(in, "Bad null? arity"));
		return in->false_;
	}
	first = tl_eval(in, first);
	return _boolify(!first);
}

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
