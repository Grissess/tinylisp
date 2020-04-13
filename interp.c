#include "tinylisp.h"

/** An internal macro for creating a new binding inside of a frame. */
#define _tl_frm_set(sm, obj, fm) tl_new_pair(in, tl_new_pair(in, tl_new_sym(in, sm), obj), fm)

/** Initialize a TinyLISP interpreter.
 *
 * This function properly initializes the fields of a `tl_interp`, after which
 * the interpreter may be considered valid, and can run evaluations. (It is
 * undefined behavior to use an interpreter before it is initialized.)
 *
 * The source for this function is a logical place to add more language
 * builtins if a module would not suffice.
 *
 * Some initialization MUST follow this; for example, TinyLISP's `tl_interp`
 * needs a valid `readf` and `writef` field before use. See the field
 * documentation for initializing those fields.
 */
void tl_interp_init(tl_interp *in) {
	in->top_alloc = NULL;
	in->true_ = tl_new_sym(in, "tl-#t");
	in->false_ = tl_new_sym(in, "tl-#f");
	in->error = NULL;
	in->prefixes = TL_EMPTY_LIST;
	in->conts = TL_EMPTY_LIST;
	in->values = TL_EMPTY_LIST;
	in->gc_events = 65536;
	in->ctr_events = 0;
	in->putback = 0;
	in->is_putback = 0;

	in->top_env = TL_EMPTY_LIST;

	tl_object *top_frm = TL_EMPTY_LIST;
	top_frm = _tl_frm_set("tl-#t", in->true_, top_frm);
	top_frm = _tl_frm_set("tl-#f", in->false_, top_frm);

	top_frm = _tl_frm_set("tl-macro", tl_new_cfunc(in, tl_cf_macro), top_frm);
	top_frm = _tl_frm_set("tl-lambda", tl_new_cfunc(in, tl_cf_lambda), top_frm);
	top_frm = _tl_frm_set("tl-define", tl_new_cfunc(in, tl_cf_define), top_frm);
	top_frm = _tl_frm_set("tl-display", tl_new_cfunc_byval(in, tl_cfbv_display), top_frm);
	top_frm = _tl_frm_set("tl-prefix", tl_new_cfunc(in, tl_cf_prefix), top_frm);
	top_frm = _tl_frm_set("tl-error", tl_new_cfunc_byval(in, tl_cfbv_error), top_frm);
	top_frm = _tl_frm_set("tl-set!", tl_new_cfunc(in, tl_cf_set), top_frm);
	top_frm = _tl_frm_set("tl-env", tl_new_cfunc_byval(in, tl_cfbv_env), top_frm);
	top_frm = _tl_frm_set("tl-set-env!", tl_new_cfunc_byval(in, tl_cfbv_setenv), top_frm);
	top_frm = _tl_frm_set("tl-top-env", tl_new_cfunc_byval(in, tl_cfbv_topenv), top_frm);
	top_frm = _tl_frm_set("tl-type", tl_new_cfunc_byval(in, tl_cfbv_type), top_frm);

	top_frm = _tl_frm_set("tl-cons", tl_new_cfunc_byval(in, tl_cfbv_cons), top_frm);
	top_frm = _tl_frm_set("tl-car", tl_new_cfunc_byval(in, tl_cfbv_car), top_frm);
	top_frm = _tl_frm_set("tl-cdr", tl_new_cfunc_byval(in, tl_cfbv_cdr), top_frm);
	top_frm = _tl_frm_set("tl-null?", tl_new_cfunc_byval(in, tl_cfbv_null), top_frm);
	top_frm = _tl_frm_set("tl-if", tl_new_cfunc(in, tl_cf_if), top_frm);

	top_frm = _tl_frm_set("tl-concat", tl_new_cfunc_byval(in, tl_cfbv_concat), top_frm);
	top_frm = _tl_frm_set("tl-length", tl_new_cfunc_byval(in, tl_cfbv_length), top_frm);
	top_frm = _tl_frm_set("tl-ord", tl_new_cfunc_byval(in, tl_cfbv_ord), top_frm);
	top_frm = _tl_frm_set("tl-chr", tl_new_cfunc_byval(in, tl_cfbv_chr), top_frm);

	top_frm = _tl_frm_set("tl-readc", tl_new_cfunc_byval(in, tl_cfbv_readc), top_frm);
	top_frm = _tl_frm_set("tl-putbackc", tl_new_cfunc_byval(in, tl_cfbv_putbackc), top_frm);
	top_frm = _tl_frm_set("tl-writec", tl_new_cfunc_byval(in, tl_cfbv_writec), top_frm);

	top_frm = _tl_frm_set("tl-+", tl_new_cfunc_byval(in, tl_cfbv_add), top_frm);
	top_frm = _tl_frm_set("tl--", tl_new_cfunc_byval(in, tl_cfbv_sub), top_frm);
	top_frm = _tl_frm_set("tl-*", tl_new_cfunc_byval(in, tl_cfbv_mul), top_frm);
	top_frm = _tl_frm_set("tl-/", tl_new_cfunc_byval(in, tl_cfbv_div), top_frm);
	top_frm = _tl_frm_set("tl-%", tl_new_cfunc_byval(in, tl_cfbv_mod), top_frm);

	top_frm = _tl_frm_set("tl-=", tl_new_cfunc_byval(in, tl_cfbv_eq), top_frm);
	top_frm = _tl_frm_set("tl-<", tl_new_cfunc_byval(in, tl_cfbv_less), top_frm);
	top_frm = _tl_frm_set("tl-nand", tl_new_cfunc_byval(in, tl_cfbv_nand), top_frm);

	top_frm = _tl_frm_set("tl-eval-in&", tl_new_cfunc_byval(in, tl_cfbv_evalin), top_frm);
	top_frm = _tl_frm_set("tl-call-with-current-continuation", tl_new_cfunc_byval(in, tl_cfbv_call_with_current_continuation), top_frm);

	/*
	top_frm = _tl_frm_set("tl-apply", tl_new_cfunc(in, tl_cf_apply), top_frm);
	*/

	top_frm = _tl_frm_set("tl-read", tl_new_cfunc_byval(in, tl_cfbv_read), top_frm);
	top_frm = _tl_frm_set("tl-gc", tl_new_cfunc_byval(in, tl_cfbv_gc), top_frm);

	top_frm = _tl_frm_set("tl-load-mod", tl_new_cfunc_byval(in, tl_cfbv_load_mod), top_frm);

#ifdef DEBUG
	top_frm = _tl_frm_set("tl-debug-print", tl_new_cfunc(in, tl_cf_debug_print), top_frm);
#endif

	in->top_env = tl_new_pair(in, top_frm, in->top_env);
	in->env = in->top_env;
}

/** Finalizes a module.
 *
 * For the most part, this frees all memory allocated by the interpreter,
 * leaving many of its pointers dangling. It is undefined behavior to use an
 * interpreter after it has been finalized.
 */
void tl_interp_cleanup(tl_interp *in) {
	while(in->top_alloc) {
		tl_free(in, in->top_alloc);
	}
}
