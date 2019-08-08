#include "tinylisp.h"

TL_MOD_INIT(tl_interp *in, const char *fname) {
	tl_printf(in, "Hello from io!\n");
	return 1;
}
