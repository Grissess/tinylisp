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

tl_object *tl_new_cfunc(tl_interp *in, tl_object *(*cfunc)(tl_interp *, tl_object *)) {
	tl_object *obj = tl_new(in);
	obj->kind = TL_CFUNC;
	obj->cfunc = cfunc;
	return obj;
}

tl_object *tl_new_func(tl_interp *in, tl_object *args, tl_object *body, tl_object *env) {
	tl_object *obj = tl_new(in);
	assert(tl_is_pair(args));
	for(tl_list_iter(args, arg)) {
		assert(tl_is_sym(arg));
	}
	assert(tl_is_pair(body));
	assert(tl_is_pair(env));
	obj->kind = TL_FUNC;
	obj->args = args;
	obj->body = body;
	obj->env = env;
	return obj;
}

tl_object *tl_new_macro(tl_interp *in, tl_object *args, const char *envn, tl_object *body, tl_object *env) {
	tl_object *obj = tl_new_func(in, args, body, env);
	obj->envn = strdup(envn);
	obj->kind = TL_MACRO;
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
	free(obj);
}

static void _tl_mark_pass(tl_object *obj) {
	if(!obj) return;
	if(tl_is_marked(obj)) return;
	tl_mark(obj);
	switch(obj->kind) {
		case TL_INT:
		case TL_SYM:
		case TL_CFUNC:
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
