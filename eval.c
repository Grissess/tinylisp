#include <string.h>
#include <assert.h>

#include "tinylisp.h"

int tl_push_eval(tl_interp *in, tl_object *expr, tl_object *env) {
	if(tl_has_error(in)) return 0;
	if(!expr) {
		tl_error_set(in, tl_new_sym(in, "evaluate ()"));
		return 0;
	}
	if(tl_is_int(expr) || tl_is_callable(expr)) {  /* Self-valuating */
		tl_values_push(in, expr);
		return 0;
	}
	if(tl_is_sym(expr)) {  /* Variable binding */
		tl_object *binding = tl_env_get_kv(env, expr->str);
		if(!binding) {
			tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "unknown var"), expr));
			return 0;
		}
		tl_values_push(in, tl_next(binding));
		return 0;
	}
	if(tl_is_pair(expr)) {  /* Application */
		size_t len = tl_list_len(expr);
		for(tl_list_iter(expr, subex)) {
			if(subex == tl_first(expr)) {
				tl_push_apply(in, (long) len - 1, subex, env);
			} else {
				tl_values_push(in, subex);
			}
		}
		return 1;
	}
	tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "unevaluable"), expr));
	return 0;
}

void tl_push_apply(tl_interp *in, long len, tl_object *expr, tl_object *env) {
	in->conts = tl_new_pair(in, tl_new_pair(in, tl_new_int(in, len), tl_new_pair(in, expr, env)), in->conts);
}

void _tl_apply_next_body_callable_k(tl_interp *in, tl_object *args, void *_cont) {
	tl_object *cont = _cont;
	tl_object *callex = tl_first(tl_next(cont));
	tl_object *env = tl_next(tl_next(cont));
	long paramlen = tl_list_len(callex->args);
	if(tl_is_macro(callex) ? (tl_list_len(args) < paramlen) : (tl_list_len(args) != paramlen)) {
		tl_error_set(in, tl_new_pair(in, tl_new_pair(in, tl_new_sym(in, "bad arity"), tl_new_int(in, paramlen)), args));
		tl_cfunc_return(in, in->false_);
	}
	tl_object *frm = TL_EMPTY_LIST;
	for(tl_object *acur = callex->args; acur && args; acur = tl_next(acur)) {
		frm = tl_new_pair(in, tl_new_pair(in, tl_first(acur), (tl_is_func(callex) || tl_next(acur)) ? tl_first(args) : args), frm);
		args = tl_next(args);
	}
	if(callex->envn) frm = tl_new_pair(in, tl_new_pair(in, tl_new_sym(in, callex->envn), env), frm);
	env = tl_new_pair(in, frm, callex->env);
	tl_object *body_rvs = tl_list_rvs(in, callex->body);
	for(tl_list_iter(body_rvs, ex)) {
		tl_push_apply(in, ex == tl_first(body_rvs) ? TL_APPLY_PUSH_EVAL : TL_APPLY_DROP_EVAL, ex, env);
	}
}

int tl_apply_next(tl_interp *in) {
	tl_object *cont = tl_first(in->conts);
	long len;
	tl_object *callex, *env, *args = TL_EMPTY_LIST;
	/*
	in->printf(in->udata, "Conts: ");
	tl_print(in, in->conts);
	in->printf(in->udata, "\n");
	*/
	if(tl_has_error(in)) return 0;
	if(!cont) return 0;
	in->conts = tl_next(in->conts);
	assert(tl_is_int(tl_first(cont)));
	len = tl_first(cont)->ival;
	callex = tl_first(tl_next(cont));
	env = tl_next(tl_next(cont));
	/*
	in->printf(in->udata, "Apply Next len %ld Callex: ", len);
	tl_print(in, callex);
	in->printf(in->udata, " ");
	*/
	if(len == TL_APPLY_DROP) {
		in->values = tl_next(in->values);
		return 1;
	}
	if(len != TL_APPLY_INDIRECT) {
		if(tl_push_eval(in, callex, env)) {
			if(!(len == TL_APPLY_PUSH_EVAL || len == TL_APPLY_DROP_EVAL)) {
				/* in->printf(in->udata, "[indirected]\n"); */
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
			return 1;
		}
	} else {
		/* in->printf(in->udata, "[resuming indirect]"); */
		len = tl_first(tl_next(cont))->ival;
	}
	/* in->printf(in->udata, "\n"); */
	tl_values_pop_into(in, callex);
	/*
	in->printf(in->udata, "Apply Next: %ld call: ", len);
	tl_print(in, callex);
	in->printf(in->udata, " values: ");
	tl_print(in, in->values);
	in->printf(in->udata, " env: ");
	tl_print(in, env);
	in->printf(in->udata, " stack: ");
	for(tl_list_iter(in->conts, ct)) {
		tl_print(in, tl_new_pair(in, tl_first(ct), tl_first(tl_next(ct))));
	}
	in->printf(in->udata, "\n");
	*/
	if(len == TL_APPLY_DROP_EVAL) return 1;
	if(len == TL_APPLY_PUSH_EVAL) {
		tl_values_push(in, callex);
		return 1;
	}
	if(!tl_is_callable(callex)) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "call non-callable"), callex));
		return 0;
	}
	for(int i = 0; i < len; i++) {
		args = tl_new_pair(in, tl_first(in->values), args);
		in->values = tl_next(in->values);
	}
	switch(callex->kind) {
		case TL_CFUNC:
			in->env = env;
			callex->cfunc(in, args, callex->state);
			break;

		case TL_FUNC:
			in->env = env;
			tl_eval_all_args(in, args, tl_new_pair(in, tl_new_int(in, len), tl_new_pair(in, callex, env)), _tl_apply_next_body_callable_k);
			break;

		case TL_MACRO:
			_tl_apply_next_body_callable_k(in, args, tl_new_pair(in, tl_new_int(in, len), tl_new_pair(in, callex, env)));
			break;

		case TL_CONT:
			if(len != 1) {
				tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "bad cont arity (1)"), args));
				return 0;
			}
			in->conts = callex->ret_cont;
			in->values = callex->ret_values;
			in->env = callex->ret_env;
			tl_push_eval(in, tl_first(args), env);
			break;

		default:
			assert(0);
			break;
	}
	return 1;
}

void _tl_eval_and_then(tl_interp *in, tl_object *expr, void *state, void (*then)(tl_interp *, tl_object *, void *), const char *name) {
	tl_object *tobj = tl_new_then(in, then, state, name);
	tl_push_apply(in, 1, tobj, in->env);
	tl_push_eval(in, expr, in->env);
}

void _tl_eval_all_args_k(tl_interp *in, tl_object *result, void *_state) {
	tl_object *state = _state;
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
		tl_eval_and_then(in, tl_first(args), new_state, _tl_eval_all_args_k);
	} else {
		for(tl_list_iter(tl_list_rvs(in, stack), elem)) {
			tl_values_push(in, elem);
		}
		tl_push_apply(in, tl_list_len(stack), then, in->env);
	}
}

void _tl_eval_all_args(tl_interp *in, tl_object *args, void *state, void (*then)(tl_interp *, tl_object *, void *), const char *name) {
	tl_object *tobj = tl_new_then(in, then, state, name);
	if(args) {
		tl_object *state = tl_new_pair(in,
			tl_new_pair(in, 
				tl_new_pair(in, tl_next(args), TL_EMPTY_LIST),
				TL_EMPTY_LIST
			),
			tobj
		);
		tl_eval_and_then(in, tl_first(args), state, _tl_eval_all_args_k);
	} else {
		tl_push_apply(in, 0, tl_new_then(in, then, state, name), in->env);
	}
}
