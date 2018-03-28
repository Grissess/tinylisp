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
		val = tl_eval(&in, expr);
		if(in.error) {
			fprintf(stderr, "Error: ");
			tl_print(&in, in.error);
			fprintf(stderr, "\n");
			tl_error_clear(&in);
		}
		fprintf(stderr, "Value: ");
		tl_print(&in, val);
		fprintf(stderr, "\n");
		tl_gc(&in);
	}
}
