#include <string.h>
#include <stdio.h>

#include "tinylisp.h"

tl_object *tl_env_get_kv(tl_interp *in, tl_object *env, tl_object *nm) {
	for(tl_list_iter(env, frame)) {
		for(tl_list_iter(frame, kv)) {
			tl_object *key = tl_first(kv);
			tl_object *val = tl_next(kv);
			if(key && tl_is_sym(key) && tl_sym_eq(key, nm)) {
				return kv;
			}
		}
	}
	return NULL;
}

tl_object *tl_env_set_global(tl_interp *in, tl_object *env, tl_object *nm, tl_object *val) {
	tl_object *kv = tl_env_get_kv(in, env, nm);
	if(kv && tl_is_pair(kv)) {
		kv->next = val;
		return env;
	}
	if(!env) {
		env = tl_new_pair(in, TL_EMPTY_LIST, env);
	}
	for(tl_list_iter(env, frame)) {
		if(!tl_next(l_frame)) {
			l_frame->first = tl_frm_set(in, frame, nm, val);
		}
	}
	return env;
}

tl_object *tl_env_set_local(tl_interp *in, tl_object *env, tl_object *nm, tl_object *val) {
	if(!env) {
		env = tl_new_pair(in, TL_EMPTY_LIST, env);
	}
	env->first = tl_frm_set(in, tl_first(env), nm, val);
	return env;
}

tl_object *tl_frm_set(tl_interp *in, tl_object *frm, tl_object *nm, tl_object *val) {
	for(tl_list_iter(frm, kv)) {
		if(kv && tl_is_pair(kv) && tl_is_sym(tl_first(kv)) && tl_sym_eq(tl_first(kv), nm)) {
			kv->next = val;
			return frm;
		}
	}
	return tl_new_pair(in, tl_new_pair(in, nm, val), frm);
}
