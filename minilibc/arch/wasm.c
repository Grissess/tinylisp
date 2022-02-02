#include <stddef.h>

#include "../arch.h"
#include "tinylisp.h"

#ifndef WASM_BUFSIZE
#define WASM_BUFSIZE 256
#endif

/* Utilities only used by the interface */

tl_object *tl_wasm_get_error(tl_interp *in) {
	return in->error;
}

tl_object *tl_wasm_get_values(tl_interp *in) {
	return in->values;
}

tl_object *tl_wasm_get_conts(tl_interp *in) {
	return in->conts;
}

tl_object *tl_wasm_get_env(tl_interp *in) {
	return in->env;
}

void tl_wasm_make_permanent(tl_object *obj) {
	tl_make_permanent(obj);
}

void tl_wasm_clear_state(tl_interp *in) {
	in->error = NULL;
	in->conts = TL_EMPTY_LIST;
	in->values = TL_EMPTY_LIST;
}

void tl_wasm_values_push(tl_interp *in, tl_object *val) {
	tl_values_push(in, val);
}
