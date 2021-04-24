#include <stdio.h>
#include <stdarg.h>

#include "tinylisp.h"

tl_interp *_global_in;

#ifdef CONFIG_MODULES
#include <dlfcn.h>
int my_modloadf(tl_interp *in, const char *fname) {
	void *hdl = dlopen(fname, RTLD_NOW | RTLD_GLOBAL);
	if(!hdl) {
		tl_printf(in, "Module load error: %s\n", dlerror());
		return 0;
	}
	void *ini = dlsym(hdl, "tl_init");
	if(!ini) {
		tl_printf(in, "Module init error: %s\n", dlerror());
		return 0;
	}
	return (*(int (**)(tl_interp *, const char *))(&ini))(in, fname);
}
#endif

void _main_k(tl_interp *in, tl_object *result, tl_object *_) {
	fprintf(stderr, "Value: ");
	tl_print(in, tl_first(result));
	fflush(stdout);
	fprintf(stderr, "\n");
	if(in->values) {
		fprintf(stderr, "(Rest of stack: ");
		tl_print(in, in->values);
		fflush(stdout);
		fprintf(stderr, ")\n");
	}
	tl_cfunc_return(in, in->true_);
}

#ifdef CONFIG_MODULES_BUILTIN
extern void *__start_tl_bmcons;
extern void *__stop_tl_bmcons;
#endif

int main() {
	tl_interp in;
	tl_object *expr, *val;

	_global_in = &in;

	tl_interp_init(&in);
#ifdef CONFIG_MODULES
	in.modloadf = my_modloadf;
#endif
#ifdef CONFIG_MODULES_BUILTIN
	{
		int (**fp)(tl_interp *, const char *) = (int (**)(tl_interp *, const char *))&__start_tl_bmcons;
		while(fp != (int (**)(tl_interp *, const char *))&__stop_tl_bmcons)
			(*fp++)(&in, NULL);
	}
#endif

	fprintf(stderr, "Top Env: ");
	tl_print(&in, in.top_env);
	fflush(stdout);
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
		fflush(stdout);
		fprintf(stderr, "\n");
		tl_eval_and_then(&in, expr, NULL, _main_k);
		tl_run_until_done(&in);
		if(in.error) {
			fprintf(stderr, "Error: ");
			tl_print(&in, in.error);
			fflush(stdout);
			fprintf(stderr, "\n");
			tl_error_clear(&in);
		}
		in.conts = TL_EMPTY_LIST;
		in.values = TL_EMPTY_LIST;  /* Expected: (tl-#t) due to _main_k */
		tl_gc(&in);
	}
}
