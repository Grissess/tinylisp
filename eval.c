#include <string.h>
#include <assert.h>

#include "tinylisp.h"

int tl_push_eval(tl_interp *in, tl_object *expr, tl_object *env) {
	if(tl_has_error(in)) return TL_RESULT_DONE;
	if(!expr) {
		tl_error_set(in, tl_new_sym(in, "evaluate ()"));
		return TL_RESULT_DONE;
	}
	if(tl_is_int(expr) || tl_is_callable(expr)) {  /* Self-valuating */
		tl_values_push(in, expr);
		return TL_RESULT_DONE;
	}
	if(tl_is_sym(expr)) {  /* Variable binding */
		tl_object *binding = tl_env_get_kv(in, env, expr);
		if(!binding) {
			tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "unknown var"), expr));
			return TL_RESULT_DONE;
		}
		tl_values_push(in, tl_next(binding));
		return TL_RESULT_DONE;
	}
	if(tl_is_pair(expr)) {  /* Application */
		size_t len = tl_list_len(expr);
		for(tl_list_iter(expr, subex)) {
			if(subex == tl_first(expr)) {
				tl_push_apply(in, (long) len - 1, subex, env);
			} else {
				tl_values_push_syntactic(in, subex);
			}
		}
		return TL_RESULT_AGAIN;
	}
	tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "unevaluable"), expr));
	return TL_RESULT_DONE;
}

void tl_push_apply(tl_interp *in, long len, tl_object *expr, tl_object *env) {
	in->conts = tl_new_pair(in, tl_new_pair(in, tl_new_int(in, len), tl_new_pair(in, expr, env)), in->conts);
	in->ctr_events++;
	if(in->gc_events > 0 && in->ctr_events >= in->gc_events) {
		tl_gc(in);
		in->ctr_events = 0;
	}
}

void _tl_apply_next_body_callable_k(tl_interp *in, tl_object *args, tl_object *cont) {
	tl_object *callex = tl_first(tl_next(cont));
	tl_object *env = tl_next(tl_next(cont));
	tl_object *frm = TL_EMPTY_LIST;

	/* Handle builtins */
	if(tl_is_cfunc(callex) || tl_is_cfunc_byval(callex) || tl_is_then(callex)) {
		callex->cfunc(in, args, callex->state);
		return;
	}

	/* Determine if arguments are legal and bind them */
	if(tl_is_pair(callex->args)) {
		char is_improp = 0;
		long paramlen = 0;

		for(tl_list_iter(callex->args, item)) {
			paramlen++;
			if(tl_is_sym(tl_next(l_item))) {
				is_improp = 1;
				break;
			}
		}

		if(is_improp ? (tl_list_len(args) < paramlen) : (tl_list_len(args) != paramlen)) {
			tl_error_set(in, tl_new_pair(in, tl_new_pair(in, tl_new_sym(in, "bad arity"), tl_new_int(in, paramlen)), args));
			tl_cfunc_return(in, in->false_);
		}

		/* Bind parameters into a new env, name by name */
		for(tl_object *acur = callex->args; acur && args; acur = tl_next(acur)) {
			frm = tl_new_pair(in, tl_new_pair(in, tl_first(acur), tl_first(args)), frm);
			args = tl_next(args);
			if(!tl_is_pair(tl_next(acur))) {
				frm = tl_new_pair(in, tl_new_pair(in, tl_next(acur), args), frm);
				break;
			}
		}
	} else if(tl_is_sym(callex->args)) {
		/* Just capture all arguments into the one name in the new frame */
		frm = tl_new_pair(in, tl_new_pair(in, callex->args, args), frm);
	} else {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "bad arg kind"), callex->args));
		tl_cfunc_return(in, in->false_);
	}

	/* For macros: before losing the old one, bind the env */
	if(callex->envn) frm = tl_new_pair(in, tl_new_pair(in, callex->envn, env), frm);

	/* ...and add the frame into the env, creating a new env */
	env = tl_new_pair(in, frm, callex->env);

	/* Push the body (in reverse order) onto the cont stack */
	tl_object *body_rvs = tl_list_rvs(in, callex->body);
	for(tl_list_iter(body_rvs, ex)) {
		tl_push_apply(in, ex == tl_first(body_rvs) ? TL_APPLY_PUSH_EVAL : TL_APPLY_DROP_EVAL, ex, env);
	}
}

int tl_apply_next(tl_interp *in) {
	tl_object *cont = tl_first(in->conts);
	long len;
	tl_object *callex, *env, *args = TL_EMPTY_LIST;
	int res;
	/*
	tl_printf(in, "Conts: ");
	tl_print(in, in->conts);
	tl_printf(in, "\n");
	*/
	if(tl_has_error(in)) {
		tl_object *rescue = tl_rescue_peek(in);
		if(!rescue) return TL_RESULT_DONE;
		tl_rescue_drop(in);
		/* Trampoline into the rescue continuation--we could do this directly,
		 * but why reinvent the wheel? */
		tl_push_apply(in, 1, rescue, in->env);
		tl_values_push(in, in->error);
		tl_error_clear(in);
		return TL_RESULT_AGAIN;
	}
	in->current = cont;
	if(!cont) return TL_RESULT_DONE;
	in->conts = tl_next(in->conts);
	assert(tl_is_int(tl_first(cont)));
	len = tl_first(cont)->ival;
	callex = tl_first(tl_next(cont));
	env = tl_next(tl_next(cont));
#ifdef CONT_DEBUG
	tl_printf(in, "Apply Next len %ld Callex: ", len);
	tl_print(in, callex);
	tl_printf(in, " ");
#endif
	if(len == TL_APPLY_DROP) {
		in->values = tl_next(in->values);
		return TL_RESULT_AGAIN;
	}
	if(len == TL_APPLY_DROP_RESCUE) {
		tl_rescue_drop(in);
		return TL_RESULT_AGAIN;
	}
	if(len == TL_APPLY_GETCHAR) {
		if(in->is_putback) {
			tl_values_push(in, tl_new_int(in, in->putback));
			in->is_putback = 0;
			return TL_RESULT_AGAIN;
		} else {
			return TL_RESULT_GETCHAR;
		}
	}
	if(len != TL_APPLY_INDIRECT) {
		if((res = tl_push_eval(in, callex, env))) {
			if(!(len == TL_APPLY_PUSH_EVAL || len == TL_APPLY_DROP_EVAL)) {
#ifdef CONT_DEBUG
				tl_printf(in, "[indirected]\n");
#endif
				cont = tl_first(in->conts);
				in->conts = tl_next(in->conts);
				tl_push_apply(in, TL_APPLY_INDIRECT, tl_new_int(in, len), env);
				in->conts = tl_new_pair(in, cont, in->conts);
			} else if(len == TL_APPLY_DROP_EVAL) {
				cont = tl_first(in->conts);
				in->conts = tl_next(in->conts);
				tl_push_apply(in, TL_APPLY_DROP, TL_EMPTY_LIST, TL_EMPTY_LIST);
				in->conts = tl_new_pair(in, cont, in->conts);
			}
			return res;
		}
	} else {
#ifdef CONT_DEBUG
		tl_printf(in, "[resuming indirect]");
#endif
		len = tl_first(tl_next(cont))->ival;
	}
#ifdef CONT_DEBUG
	tl_printf(in, "\n");
#endif
	tl_values_pop_into(in, callex);
#ifdef CONT_DEBUG
	tl_printf(in, "Apply Next: %ld call: ", len);
	tl_print(in, callex);
	tl_printf(in, " values: ");
	tl_print(in, in->values);
	/*
	tl_printf(in, " env: ");
	tl_print(in, env);
	*/
	tl_printf(in, " stack: ");
	for(tl_list_iter(in->conts, ct)) {
		tl_print(in, tl_new_pair(in, tl_first(ct), tl_first(tl_next(ct))));
	}
	tl_printf(in, "\n");
#endif
	if(len == TL_APPLY_DROP_EVAL) return TL_RESULT_AGAIN;
	if(len == TL_APPLY_PUSH_EVAL) {
		tl_values_push(in, callex);
		return TL_RESULT_AGAIN;
	}
	if(!tl_is_callable(callex)) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "call non-callable"), callex));
		return TL_RESULT_AGAIN;
	}
	for(int i = 0; i < len; i++) {
		args = tl_new_pair(in, tl_first(in->values), args);
		in->values = tl_next(in->values);
	}
	in->env = env;
	tl_object *new_args = TL_EMPTY_LIST;
	switch(callex->kind) {
		case TL_FUNC:
		case TL_CFUNC_BYVAL:
			tl_eval_all_args(in, args, tl_new_pair(in, tl_new_int(in, len), tl_new_pair(in, callex, env)), _tl_apply_next_body_callable_k);
			break;

		case TL_MACRO:
		case TL_CFUNC:
		case TL_THEN:
			for(tl_list_iter(args, arg)) {
				if(callex->kind != TL_THEN && tl_next(arg) != in->true_) {
					tl_error_set(in, tl_new_pair(in, tl_new_pair(in, tl_new_sym(in, "invoke macro/cfunc with non-syntactic arg"), callex), arg));
					return TL_RESULT_AGAIN;
				}
				new_args = tl_new_pair(in, tl_first(arg), new_args);
			}
			_tl_apply_next_body_callable_k(in, tl_list_rvs(in, new_args), tl_new_pair(in, tl_new_int(in, len), tl_new_pair(in, callex, env)));
			break;

		case TL_CONT:
			if(len != 1) {
				tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "bad cont arity (1)"), args));
				return TL_RESULT_AGAIN;
			}
			in->conts = callex->ret_conts;
			in->values = callex->ret_values;
			in->env = callex->ret_env;
			if(tl_next(tl_first(args)) == in->true_) {
				tl_push_eval(in, tl_first(tl_first(args)), env);
			} else {
				tl_values_push(in, tl_first(tl_first(args)));
			}
			break;

		default:
			assert(0);
			break;
	}
	return TL_RESULT_AGAIN;
}

void _tl_eval_and_then(tl_interp *in, tl_object *expr, tl_object *state, void (*then)(tl_interp *, tl_object *, tl_object *), const char *name) {
	tl_object *tobj = tl_new_then(in, then, state, name);
	tl_push_apply(in, 1, tobj, in->env);
	tl_push_eval(in, expr, in->env);
}

void _tl_getc_and_then(tl_interp *in, tl_object *state, void (*then)(tl_interp *, tl_object *, tl_object *), const char *name) {
	tl_object *tobj = tl_new_then(in, then, state, name);
	tl_push_apply(in, 1, tobj, in->env);
	tl_push_apply(in, TL_APPLY_GETCHAR, TL_EMPTY_LIST, TL_EMPTY_LIST);
}

void _tl_eval_all_args_k(tl_interp *in, tl_object *result, tl_object *state) {
	tl_object *args = tl_first(tl_first(tl_first(state)));
	tl_object *stack = tl_new_pair(in, tl_first(result), tl_next(tl_first(state)));
	tl_object *then = tl_next(state);
	tl_object *new_state = tl_new_pair(in,
		tl_new_pair(in,
			tl_new_pair(in, tl_next(args), TL_EMPTY_LIST),
			stack
		),
		then
	);
	if(args) {
		if(tl_next(tl_first(args)) == in->true_) {
			tl_eval_and_then(in, tl_first(tl_first(args)), new_state, _tl_eval_all_args_k);
		} else {
			tl_values_push(in, tl_first(tl_first(args)));
			tl_push_apply(in, 1, tl_new_then(in, _tl_eval_all_args_k, new_state, "_tl_apply_all_args_k<indirect>"), in->env);
		}
	} else {
		for(tl_list_iter(tl_list_rvs(in, stack), elem)) {
			tl_values_push(in, elem);
		}
		tl_push_apply(in, tl_list_len(stack), then, in->env);
	}
}

void _tl_eval_all_args(tl_interp *in, tl_object *args, tl_object *state, void (*then)(tl_interp *, tl_object *, tl_object *), const char *name) {
	tl_object *tobj = tl_new_then(in, then, state, name);
	if(args) {
		tl_object *state = tl_new_pair(in,
			tl_new_pair(in, 
				tl_new_pair(in, tl_next(args), TL_EMPTY_LIST),
				TL_EMPTY_LIST
			),
			tobj
		);
		if(tl_next(tl_first(args)) == in->true_) {
			tl_eval_and_then(in, tl_first(tl_first(args)), state, _tl_eval_all_args_k);
		} else {
			tl_values_push(in, tl_first(tl_first(args)));
			tl_push_apply(in, 1, tl_new_then(in, _tl_eval_all_args_k, state, "_tl_apply_all_args_k<direct>"), in->env);
		}
	} else {
		tl_push_apply(in, 0, tl_new_then(in, then, state, name), in->env);
	}
}

/** Runs an interpreter with one or more pushed evaluations until all evaulation is finished or an error occurs.
 *
 * This simply calls `tl_apply_next` in a loop until it returns
 * `TL_RESULT_DONE`, handing `TL_RESULT_GETCHAR` by using the interpreter's
 * tl_interp::readf.
 */
void tl_run_until_done(tl_interp *in) {
	int res;
	while((res = tl_apply_next(in))) {
		switch(res) {
			case TL_RESULT_AGAIN:
				break;

			case TL_RESULT_GETCHAR:
				tl_values_push(in, tl_new_int(in, (long)tl_getc(in)));
				break;

			default:
				assert(0);
				break;
		}
	}
}
