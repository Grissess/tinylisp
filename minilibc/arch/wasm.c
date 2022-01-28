#include <stddef.h>

#include "../arch.h"
#include "tinylisp.h"

/* Utilities only used by the interface */

tl_object *tl_wasm_get_error(tl_interp *in) {
	return in->error;
}

void tl_wasm_clear_state(tl_interp *in) {
	in->error = NULL;
	in->conts = TL_EMPTY_LIST;
	in->values = TL_EMPTY_LIST;
}
