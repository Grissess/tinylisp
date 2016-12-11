#include <string.h>

#include "tinylisp.h"

tl_object *tl_eval(tl_interp *in, tl_object *obj) {
	if(!obj) {
		tl_error_set(in, tl_new_sym("Attempt to eval NULL"));
		return in->false_;
	}
	/* Self-valuating */
	if(tl_is_int(obj)) {
		return obj;
	}
	/* Variable bindings */
	if(tl_is_sym(obj)) {
		tl_object *val = tl_env_get_kv(in->env, obj->str);
		if(!val) {
			tl_error_set(in, tl_new_pair(tl_new_sym("Unbound var"), obj));
			return in->false_;
		}
		return tl_next(val);
	}
	/* Everything else (legal) */
	if(tl_is_pair(obj)) {
		tl_object *first = tl_first(obj);
		tl_object *next = tl_next(obj);
		if(!first) {
			tl_error_set(in, tl_new_sym("Eval empty expression"));
			return in->false_;
		}
		if(tl_is_sym(first)) {
			/* "special" function handling */
			/* Quotations */
			if(!strcmp(first->str, "quote")) {
				return tl_first(next);
			}
		}
		tl_object *callable = tl_eval(in, first);
		return tl_apply(in, tl_new_pair(callable, next));
	}
	tl_error_set(in, tl_new_pair(tl_new_sym("Unevaluable"), obj));
	return in->false_;
}

tl_object *tl_apply(tl_interp *in, tl_object *args) {
	tl_object *callable = tl_first(args), *fargs = tl_next(args);
	tl_object *old_env = in->env;
	tl_object *frm = TL_EMPTY_LIST;
	if(!callable) {
		tl_error_set(in, tl_new_sym("NULL (or empty list) callable"));
		return in->false_;
	}
	switch(callable->kind) {
		case TL_CFUNC:
			return callable->cfunc(in, fargs);
			break;

		case TL_FUNC:
		case TL_MACRO:
			in->env = tl_new_pair(callable->env, in->env);
			if(
				callable->kind == TL_FUNC && tl_list_len(fargs) != tl_list_len(callable->args)
			||	callable->kind == TL_MACRO && tl_list_len(fargs) < tl_list_len(callable->args)) {
				tl_error_set(in, tl_new_pair(tl_new_pair(tl_new_pair(tl_new_sym("Bad arity"), TL_EMPTY_LIST), tl_new_pair(args, TL_EMPTY_LIST)), tl_new_pair(fargs, TL_EMPTY_LIST)));
				return in->false_;
			}
			for(
				tl_object *fcur = fargs, *acur = callable->args;
				fcur && acur;
				fcur = tl_next(fcur), acur = tl_next(acur)
			) {
				if(callable->kind == TL_MACRO && !tl_next(acur)) {
					frm = tl_new_pair(tl_new_pair(tl_first(acur), fcur), frm);
				} else {
					tl_object *val = tl_first(fcur);
					if(callable->kind == TL_FUNC) {
						val = tl_eval(in, val);
					}
					frm = tl_new_pair(tl_new_pair(tl_first(acur), val), frm);
				}
			}
			in->env = tl_new_pair(frm, in->env);
			tl_object *res = in->false_;
			for(tl_list_iter(callable->body, expr)) {
				res = tl_eval(in, expr);
			}
			in->env = old_env;
			return res;
			break;
	}
	tl_error_set(in, tl_new_pair(tl_new_sym("Not callable"), callable));
	return in->false_;
}
