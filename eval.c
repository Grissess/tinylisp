#include <string.h>

#include "tinylisp.h"

tl_object *tl_eval(tl_interp *in, tl_object *obj) {
	if(!obj) {
		tl_error_set(in, tl_new_sym(in, "Attempt to eval NULL"));
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
			tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "Unbound var"), obj));
			return in->false_;
		}
		return tl_next(val);
	}
	/* Everything else (legal) */
	if(tl_is_pair(obj)) {
		tl_object *first = tl_first(obj);
		tl_object *next = tl_next(obj);
		if(!first) {
			tl_error_set(in, tl_new_sym(in, "Eval empty expression"));
			return in->false_;
		}
		tl_object *callable = tl_eval(in, first);
		return tl_apply(in, tl_new_pair(in, callable, next));
	}
	tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "Unevaluable"), obj));
	return in->false_;
}

tl_object *tl_apply(tl_interp *in, tl_object *args) {
	tl_object *callable = tl_first(args), *fargs = tl_next(args);
	tl_object *old_env = in->env;
	tl_object *frm = TL_EMPTY_LIST;
	if(!callable) {
		tl_error_set(in, tl_new_sym(in, "NULL (or empty list) callable"));
		return in->false_;
	}
	switch(callable->kind) {
		case TL_CFUNC:
			return callable->cfunc(in, fargs);
			break;

		case TL_FUNC:
		case TL_MACRO:
			if(
				tl_is_func(callable)&& tl_list_len(fargs) != tl_list_len(callable->args)
			||	tl_is_macro(callable) && tl_list_len(fargs) < tl_list_len(callable->args)) {
				tl_error_set(in, tl_new_pair(in, tl_new_pair(in, tl_new_pair(in, tl_new_sym(in, "Bad arity"), TL_EMPTY_LIST), tl_new_pair(in, args, TL_EMPTY_LIST)), tl_new_pair(in, fargs, TL_EMPTY_LIST)));
				return in->false_;
			}
			for(
				tl_object *fcur = fargs, *acur = callable->args;
				fcur && acur;
				fcur = tl_next(fcur), acur = tl_next(acur)
			) {
				if(tl_is_macro(callable) && !tl_next(acur)) {
					frm = tl_new_pair(in, tl_new_pair(in, tl_first(acur), fcur), frm);
				} else {
					tl_object *val = tl_first(fcur);
					if(tl_is_func(callable)) {
						val = tl_eval(in, val);
					}
					frm = tl_new_pair(in, tl_new_pair(in, tl_first(acur), val), frm);
				}
			}
			if(tl_is_macro(callable)) {
				frm = tl_new_pair(in, tl_new_pair(in, tl_new_sym(in, callable->envn), in->env), frm);
			}
			/* XXX These must be postponed so that parameters are evaluated in
			 * the right context */
			//in->env = tl_new_pair(in, callable->env, in->env);
			in->env = callable->env;
			in->env = tl_new_pair(in, frm, in->env);
			//in->printf(in->udata, "Call env: ");
			//tl_print(in, in->env);
			//in->printf(in->udata, "\n\n- Old env: ");
			//tl_print(in, old_env);
			//in->printf(in->udata, "\n\n");
			tl_object *res = in->false_;
			for(tl_list_iter(callable->body, expr)) {
				res = tl_eval(in, expr);
			}
			in->env = old_env;
			return res;
			break;
	}
	tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "Not callable"), callable));
	return in->false_;
}
