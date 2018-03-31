#include <stdio.h>
#include <stdarg.h>

#include "tinylisp.h"

int my_readf(void *_) {
	return getchar();
}

void my_putbackf(void *_, int c) {
	ungetc(c, stdin);
}

void my_printf(void *_, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void _main_k(tl_interp *in, tl_object *result, void *_) {
	fprintf(stderr, "Value: ");
	tl_print(in, tl_first(result));
	fprintf(stderr, "\n");
	if(in->values) {
		fprintf(stderr, "(Rest of stack: ");
		tl_print(in, in->values);
		fprintf(stderr, ")\n");
	}
	tl_cfunc_return(in, in->true_);
}

int main() {
	tl_interp in;
	tl_object *expr, *val;

	tl_interp_init(&in);
	in.readf = my_readf;
	in.putbackf = my_putbackf;
	in.printf = my_printf;

	fprintf(stderr, "Top Env: ");
	tl_print(&in, in.top_env);
	fprintf(stderr, "\n");

	while(1) {
		fprintf(stderr, "> ");
		expr = tl_read(&in, TL_EMPTY_LIST);
		if(!expr) {
			printf("Done.\n");
			tl_interp_cleanup(&in);
			return 0;
		}
		fprintf(stderr, "Read: ");
		tl_print(&in, expr);
		fprintf(stderr, "\n");
		tl_eval_and_then(&in, expr, NULL, _main_k);
		tl_run_until_done(&in);
		if(in.error) {
			fprintf(stderr, "Error: ");
			tl_print(&in, in.error);
			fprintf(stderr, "\n");
			tl_error_clear(&in);
		}
		in.conts = TL_EMPTY_LIST;
		in.values = TL_EMPTY_LIST;  /* Expected: (tl-#t) due to _main_k */
		tl_gc(&in);
	}
}
