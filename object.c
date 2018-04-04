#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "tinylisp.h"

tl_object *tl_new(tl_interp *in) {
	tl_object *obj = malloc(sizeof(tl_object));
	assert(obj);
	obj->next_alloc = in->top_alloc;
	obj->prev_alloc = NULL;
	if(in->top_alloc) in->top_alloc->prev_alloc = obj;
	in->top_alloc = obj;
	return obj;
}

tl_object *tl_new_int(tl_interp *in, long ival) {
	tl_object *obj = tl_new(in);
	obj->kind = TL_INT;
	obj->ival = ival;
	return obj;
}

tl_object *tl_new_sym(tl_interp *in, const char *str) {
	tl_object *obj = tl_new(in);
	obj->kind = TL_SYM;
	if(str) {
		obj->str = strdup(str);
	} else {
		obj->str = NULL;
	}
	return obj;
}

tl_object *tl_new_pair(tl_interp *in, tl_object *first, tl_object *next) {
	tl_object *obj = tl_new(in);
	obj->kind = TL_PAIR;
	obj->first = first;
	obj->next = next;
	return obj;
}

tl_object *tl_new_then(tl_interp *in, void (*cfunc)(tl_interp *, tl_object *, tl_object *), tl_object *state, const char *name) {
	tl_object *obj = tl_new(in);
	obj->kind = TL_THEN;
	obj->cfunc = cfunc;
	obj->state = state;
	obj->name = name ? strdup(name) : NULL;
	return obj;
}

tl_object *_tl_new_cfunc(tl_interp *in, void (*cfunc)(tl_interp *, tl_object *, tl_object *), const char *name) {
	tl_object *obj = tl_new_then(in, cfunc, TL_EMPTY_LIST, name);
	obj->kind = TL_CFUNC;
	return obj;
}

tl_object *_tl_new_cfunc_byval(tl_interp *in, void (*cfunc)(tl_interp *, tl_object *, tl_object *), const char *name) {
	tl_object *obj = tl_new_then(in, cfunc, TL_EMPTY_LIST, name);
	obj->kind = TL_CFUNC_BYVAL;
	return obj;
}

tl_object *tl_new_macro(tl_interp *in, tl_object *args, const char *envn, tl_object *body, tl_object *env) {
	tl_object *obj = tl_new(in);
	obj->kind = envn ? TL_MACRO : TL_FUNC;
	obj->args = args;
	obj->body = body;
	obj->env = env;
	obj->envn = envn ? strdup(envn) : NULL;
	return obj;
}

tl_object *tl_new_cont(tl_interp *in, tl_object *env, tl_object *conts, tl_object *values) {
	tl_object *obj = tl_new(in);
	obj->kind = TL_CONT;
	obj->ret_env = env;
	obj->ret_conts = conts;
	obj->ret_values = values;
	return obj;
}

void tl_free(tl_interp *in, tl_object *obj) {
	if(!obj) return;
	if(obj->prev_alloc) {
		obj->prev_alloc->next_alloc = tl_make_next_alloc(
			obj->prev_alloc->next_alloc,
			tl_next_alloc(obj)
		);
	} else {
		in->top_alloc = tl_make_next_alloc(
			in->top_alloc,
			tl_next_alloc(obj)
		);
	}
	if(tl_next_alloc(obj)) {
		tl_next_alloc(obj)->prev_alloc = obj->prev_alloc;
	}
	switch(obj->kind) {
		case TL_SYM:
			free(obj->str);
			break;

		case TL_MACRO:
			free(obj->envn);
			break;

		case TL_CFUNC:
		case TL_CFUNC_BYVAL:
		case TL_THEN:
			free(obj->name);
			break;

		default:
			break;
	}
	free(obj);
}

static void _tl_mark_pass(tl_object *obj) {
	if(!obj) return;
	if(tl_is_marked(obj)) return;
	tl_mark(obj);
	switch(obj->kind) {
		case TL_INT:
		case TL_SYM:
			break;

		case TL_CFUNC:
		case TL_CFUNC_BYVAL:
		case TL_THEN:
			_tl_mark_pass(obj->state);
			break;

		case TL_FUNC:
		case TL_MACRO:
			_tl_mark_pass(obj->args);
			_tl_mark_pass(obj->body);
			_tl_mark_pass(obj->env);
			break;

		case TL_PAIR:
			_tl_mark_pass(obj->first);
			_tl_mark_pass(obj->next);
			break;

		case TL_CONT:
			_tl_mark_pass(obj->ret_env);
			_tl_mark_pass(obj->ret_conts);
			_tl_mark_pass(obj->ret_values);
			break;

		default:
			assert(0);
	}
}

void tl_gc(tl_interp *in) {
	tl_object *obj = in->top_alloc;
	tl_object *tmp;
	while(obj) {
		tl_unmark(obj);
		obj = tl_next_alloc(obj);
	}
	_tl_mark_pass(in->true_);
	_tl_mark_pass(in->false_);
	_tl_mark_pass(in->error);
	_tl_mark_pass(in->prefixes);
	_tl_mark_pass(in->env);
	_tl_mark_pass(in->top_env);
	_tl_mark_pass(in->conts);
	_tl_mark_pass(in->values);
	obj = in->top_alloc;
	while(obj) {
		tmp = obj;
		obj = tl_next_alloc(obj);
		if(!tl_is_marked(tmp)) {
			tl_free(in, tmp);
		}
	}
}

size_t tl_list_len(tl_object *l) {
	size_t cnt = 0;
	if(!l || !tl_is_pair(l)) {
		return cnt;
	}
	for(tl_list_iter(l, item)) {
		cnt++;
	}
	return cnt;
}

tl_object *tl_list_rvs(tl_interp *in, tl_object *l) {
	tl_object *res = TL_EMPTY_LIST;
	for(tl_list_iter(l, item)) {
		res = tl_new_pair(in, item, res);
	}
	return res;
}
