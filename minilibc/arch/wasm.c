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

void tl_wasm_clear_state(tl_interp *in) {
	in->error = NULL;
	in->conts = TL_EMPTY_LIST;
	in->values = TL_EMPTY_LIST;
}

/* TODO: use a proper intrusive queue */
int wasm_char_top = 0;
int wasm_char[WASM_BUFSIZE] = {};

int tl_wasm_putc(int ch) {
	if(wasm_char_top >= WASM_BUFSIZE - 1) return 1;
	wasm_char[wasm_char_top++] = ch;
	return 0;
}

int arch_fgetc(unsigned long fd) {
	int i, c;
	if(wasm_char_top <= 0) __builtin_trap();
	c = wasm_char[0];
	wasm_char_top--;
	for(i = 0; i < wasm_char_top; i++) {
		wasm_char[i] = wasm_char[i+1];
	}
	return c;
}
