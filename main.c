#include <stdio.h>
#include <stdarg.h>

#ifdef UNIX
#include <unistd.h>
#endif

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

#define QUIET_OFF (0)
#define QUIET_NO_PROMPT (1)
#define QUIET_NO_TRUE (2)
#define QUIET_NO_VALUE (3)
int quiet = QUIET_OFF;
#define tl_prompt(...) if(quiet == QUIET_OFF) fprintf(stderr, __VA_ARGS__)

void _main_k(tl_interp *in, tl_object *result, tl_object *_) {
	tl_prompt("Value: ");
	if(quiet != QUIET_NO_VALUE && (quiet != QUIET_NO_TRUE || tl_first(result) != in->true_)) {
		tl_print(in, tl_first(result));
		tl_printf(in, "\n");
	}
	fflush(stdout);
	if(in->values) {
		tl_prompt("(Rest of stack: ");
		tl_print(in, in->values);
		fflush(stdout);
		tl_prompt(")\n");
	}
	tl_cfunc_return(in, in->true_);
}

TL_CFBV(quiet, "quiet") {
	if(args) {
		tl_object *arg = tl_first(args);
		if(tl_is_int(arg)) {
			quiet = (int) arg->ival;
			tl_cfunc_return(in, in->true_);
		} else {
			tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "tl-quiet on non-int"), arg));
		}
	} else {
		tl_cfunc_return(in, tl_new_int(in, (long) quiet));
	}
}

#ifdef CONFIG_MODULES_BUILTIN
extern void *__start_tl_bmcons;
extern void *__stop_tl_bmcons;
#endif

int main() {
	tl_interp in;
	tl_object *expr, *val;

	_global_in = &in;

#ifdef UNIX
	if(!isatty(STDIN_FILENO)) {
		quiet = QUIET_NO_TRUE;
	}
#endif

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

	if(quiet == QUIET_OFF) {
		tl_prompt("Top Env: ");
		tl_print(&in, in.top_env);
#ifdef NS_DEBUG
		tl_prompt("Namespace:\n");
		tl_ns_print(&in, &in.ns);
#endif
		fflush(stdout);
		tl_prompt("\n");
	}

	while(1) {
		tl_prompt("> ");
		expr = tl_read(&in, TL_EMPTY_LIST);
		if(!expr) {
			tl_prompt("Done.\n");
			tl_interp_cleanup(&in);
			return 0;
		}
		if(quiet == QUIET_OFF) {
			tl_prompt("Read: ");
			tl_print(&in, expr);
			fflush(stdout);
			tl_prompt("\n");
		}
		tl_eval_and_then(&in, expr, NULL, _main_k);
		tl_run_until_done(&in);
		if(in.error) {
			/* Don't change these to tl_prompt--errors are always exceptional */
			fprintf(stderr, "Error: ");
			tl_print(&in, in.error);
			fflush(stdout);
			fprintf(stderr, "\n");
			tl_error_clear(&in);
		}
		in.conts = TL_EMPTY_LIST;
		in.values = TL_EMPTY_LIST;  /* Expected: (tl-#t) due to _main_k */
		tl_gc(&in);
#ifdef NS_DEBUG
		tl_prompt("Namespace:\n");
		tl_ns_print(&in, &in.ns);
#endif
	}
}
