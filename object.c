#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "tinylisp.h"

tl_object *tl_new() {
	return malloc(sizeof(tl_object));
}

tl_object *tl_new_int(long ival) {
	tl_object *obj = tl_new();
	obj->kind = TL_INT;
	obj->ival = ival;
	return obj;
}

tl_object *tl_new_sym(const char *str) {
	tl_object *obj = tl_new();
	obj->kind = TL_SYM;
	if(str) {
		obj->str = strdup(str);
	} else {
		obj->str = NULL;
	}
	return obj;
}

tl_object *tl_new_pair(tl_object *first, tl_object *next) {
	tl_object *obj = tl_new();
	obj->kind = TL_PAIR;
	obj->first = first;
	obj->next = next;
	return obj;
}

tl_object *tl_new_cfunc(tl_object *(*cfunc)(tl_interp *, tl_object *)) {
	tl_object *obj = tl_new();
	obj->kind = TL_CFUNC;
	obj->cfunc = cfunc;
	return obj;
}

tl_object *tl_new_func(tl_object *args, tl_object *body, tl_object *env) {
	tl_object *obj = tl_new();
	assert(!args || tl_is_pair(args));
	for(tl_list_iter(args, arg)) {
		assert(tl_is_sym(arg));
	}
	assert(!body || tl_is_pair(body));
	assert(!env || tl_is_pair(env));
	obj->kind = TL_FUNC;
	obj->args = args;
	obj->body = body;
	obj->env = env;
	return obj;
}

tl_object *tl_new_macro(tl_object *args, tl_object *body, tl_object *env) {
	tl_object *obj = tl_new_func(args, body, env);
	obj->kind = TL_MACRO;
	return obj;
}

void tl_free(tl_object *obj) {
	if(!obj) return;
	switch(obj->kind) {
		case TL_INT:
			break;

		case TL_SYM:
			free(obj->str);
			break;

		case TL_PAIR:
			tl_free(obj->first);
			tl_free(obj->next);
			break;

		case TL_CFUNC:
			break;

		case TL_FUNC:
		case TL_MACRO:
			tl_free(obj->args);
			tl_free(obj->body);
			tl_free(obj->env);
			break;

		default:
			assert(0);
	}
	free(obj);
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

tl_object *tl_list_rvs(tl_object *l) {
	tl_object *res = TL_EMPTY_LIST;
	for(tl_list_iter(l, item)) {
		res = tl_new_pair(item, res);
	}
	return res;
}
