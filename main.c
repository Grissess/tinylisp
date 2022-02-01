#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef UNIX
#include <unistd.h>
#endif

#include "tinylisp.h"

#ifdef INITSCRIPTS
extern char __start_tl_init_scripts, __stop_tl_init_scripts;

static int _postscript_readf(tl_interp *in) { return getchar(); }

static int _initscript_readf(tl_interp *in) {
	static char *ptr = &__start_tl_init_scripts;
	if(ptr >= &__stop_tl_init_scripts) {
		in->readf = _postscript_readf;
		return _postscript_readf(in);
	}
	return (int) *ptr++;
}
#endif

tl_interp *_global_in;  /* Here mostly for debugger access */

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

int running = 1;

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

void _main_read_k(tl_interp *in, tl_object *args, tl_object *_) {
	tl_object *expr = tl_first(args);
	if(!expr) {
		tl_prompt("Done.\n");
		tl_interp_cleanup(in);
		running = 0;
		tl_cfunc_return(in, in->true_);
	}
	if(quiet == QUIET_OFF) {
		tl_prompt("Read: ");
		tl_print(in, expr);
		fflush(stdout);
		tl_prompt("\n");
	}
	in->current = TL_EMPTY_LIST;
	tl_eval_and_then(in, expr, NULL, _main_k);
};

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

TL_CFBV(exit, "exit") {
	if(!args || !tl_is_int(tl_first(args))) {
		tl_error_set(in, tl_new_pair(in, tl_new_sym(in, "tl-quit on non-int"), args));
		tl_cfunc_return(in, in->false_);
	}
	exit(tl_first(args)->ival);
}

void _print_cont_stack(tl_interp *in, tl_object *stack, int level);

void _print_cont(tl_interp *in, tl_object *cont, int level) {
	tl_object *len, *callex;
	fprintf(stderr, "Len ");
	len = tl_first(cont);
	tl_print(in, len);
	fflush(stdout);
	if(tl_is_int(len) && len->ival < 0) {
		switch(len->ival) {
			case TL_APPLY_PUSH_EVAL: fprintf(stderr, " (TL_APPLY_PUSH_EVAL)"); break;
			case TL_APPLY_INDIRECT: fprintf(stderr, " (TL_APPLY_INDIRECT)"); break;
			case TL_APPLY_DROP_EVAL: fprintf(stderr, " (TL_APPLY_DROP_EVAL)"); break;
			case TL_APPLY_DROP: fprintf(stderr, " (TL_APPLY_DROP)"); break;
			case TL_APPLY_DROP_RESCUE: fprintf(stderr, " (TL_APPLY_DROP_RESCUE)"); break;
		}
	}
	fprintf(stderr, " Callex ");
	callex = tl_first(tl_next(cont));
	tl_print(in, callex);
	fflush(stdout);
	if(tl_is_then(callex) && callex->state) {
		/* I'd like to see where this is proven wrong */
		fprintf(stderr, " Returns to ");
		_print_cont(in, callex->state, level + 1);
	}
	if(tl_is_cont(callex) && !tl_is_marked(callex)) {
		tl_mark(callex);
		fprintf(stderr, ":");
		_print_cont_stack(in, callex->ret_conts, level + 1);
	}
}

void _print_cont_stack(tl_interp *in, tl_object *stack, int level) {
	int i;
	for(tl_list_iter(in->conts, cont)) {
		fprintf(stderr, "\n");
		for(i = 0; i < level; i++) fprintf(stderr, "  ");
		fprintf(stderr, "Stack");
		if(l_cont == in->conts) {
			fprintf(stderr, "(Top)");
		}
		if(!tl_next(l_cont)) {
			fprintf(stderr, "(Bottom)");
		}
		fprintf(stderr, ": ");
		_print_cont(in, cont, level);
	}
}

void print_cont_stack(tl_interp *in, tl_object *stack) {
	/* Borrow the GC marker, with care; as long as we don't run user code here,
	 * the GC won't run anyway, and we're being careful not to alloc new
	 * objects.
	 */
	tl_object *obj = in->top_alloc;
	while(obj) {
		tl_unmark(obj);
		obj = tl_next_alloc(obj);
	}

	fprintf(stderr, "\nCurrent: ");
	_print_cont(in, in->current, 0);
	_print_cont_stack(in, stack, 0);
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
#ifdef INITSCRIPTS
	in.readf = _initscript_readf;
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

	while(running) {
		tl_prompt("> ");
		tl_read_and_then(&in, _main_read_k, TL_EMPTY_LIST);
#ifdef FAKE_ASYNC
		while(1) {
			int res = tl_apply_next(&in);
			if(!res) break;
			switch(res) {
				case TL_RESULT_AGAIN: break;
				case TL_RESULT_GETCHAR:
					tl_values_push(&in, tl_new_int(&in, getchar()));
					break;
			}
		}
#else
		tl_run_until_done(&in);
#endif
		if(in.error) {
			/* Don't change these to tl_prompt--errors are always exceptional */
			fprintf(stderr, "Error: ");
			tl_print(&in, in.error);
			fflush(stdout);
			print_cont_stack(&in, in.conts);
			fprintf(stderr, "\nValues: ");
			tl_print(&in, in.values);
			fflush(stdout);
			for(tl_list_iter(in.env, frm)) {
				fprintf(stderr, "\nFrame");
				if(!tl_next(l_frm)) {
					fprintf(stderr, "(Outer)");
				}
				if(l_frm == in.env) {
					fprintf(stderr, "(Inner)");
				}
				fprintf(stderr, ": ");
				tl_print(&in, frm);
				fflush(stdout);
			}
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
