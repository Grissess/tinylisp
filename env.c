#include <string.h>
#include <stdio.h>

#include "tinylisp.h"

extern tl_interp *_global_in;

tl_object *tl_env_get_kv(tl_object *env, const char *nm) {
	for(tl_list_iter(env, frame)) {
		for(tl_list_iter(frame, kv)) {
			tl_object *key = tl_first(kv);
			tl_object *val = tl_next(kv);
			if(key && tl_is_sym(key) && !strcmp(key->str, nm)) {
				return kv;
			}
		}
	}
	return NULL;
}

tl_object *tl_env_get(tl_object *env, const char *nm) {
	tl_object *kv = tl_env_get_kv(env, nm);
	if(kv && tl_is_pair(kv)) {
		return tl_next(kv);
	}
	return NULL;
}

tl_object *tl_env_set_global(tl_interp *in, tl_object *env, const char *nm, tl_object *val) {
	tl_object *kv = tl_env_get(env, nm);
	if(kv && tl_is_pair(kv)) {
		kv->next = val;
	}
	if(!env) {
		env = tl_new_pair(in, TL_EMPTY_LIST, env);
	}
	env->first = tl_new_pair(in, tl_new_pair(in, tl_new_sym(in, nm), val), env->first);
	return env;
}

tl_object *tl_env_set_local(tl_interp *in, tl_object *env, const char *nm, tl_object *val) {
	if(!env) {
		env = tl_new_pair(in, TL_EMPTY_LIST, env);
	}
	tl_object *frm = env->first;
	for(tl_list_iter(frm, kv)) {
		if(kv && tl_is_pair(kv) && tl_is_sym(tl_first(kv)) && !strcmp(tl_first(kv)->str, nm)) {
			kv->next = val;
			return env;
		}
	}
	env->first = tl_new_pair(in, tl_new_pair(in, tl_new_sym(in, nm), val), env->first);
	return env;
}
