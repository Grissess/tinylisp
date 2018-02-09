#include "tinylisp.h"

#define _tl_frm_set(sm, obj, fm) tl_new_pair(in, tl_new_pair(in, tl_new_sym(in, sm), obj), fm)
void tl_interp_init(tl_interp *in) {
	in->top_alloc = NULL;
	in->true_ = tl_new_sym(in, "#t");
	in->false_ = tl_new_sym(in, "#f");
	in->error = NULL;
	in->prefixes = TL_EMPTY_LIST;

	in->top_env = TL_EMPTY_LIST;

	tl_object *top_frm = TL_EMPTY_LIST;
	top_frm = _tl_frm_set("tl-#t", in->true_, top_frm);
	top_frm = _tl_frm_set("tl-#f", in->false_, top_frm);

	top_frm = _tl_frm_set("tl-error", tl_new_cfunc(in, tl_cf_error), top_frm);
	top_frm = _tl_frm_set("tl-lambda", tl_new_cfunc(in, tl_cf_lambda), top_frm);
	top_frm = _tl_frm_set("tl-macro", tl_new_cfunc(in, tl_cf_macro), top_frm);
	top_frm = _tl_frm_set("tl-prefix", tl_new_cfunc(in, tl_cf_prefix), top_frm);
	top_frm = _tl_frm_set("tl-define", tl_new_cfunc(in, tl_cf_define), top_frm);
	top_frm = _tl_frm_set("tl-set!", tl_new_cfunc(in, tl_cf_set), top_frm);
	top_frm = _tl_frm_set("tl-env", tl_new_cfunc(in, tl_cf_env), top_frm);
	top_frm = _tl_frm_set("tl-set-env!", tl_new_cfunc(in, tl_cf_setenv), top_frm);
	top_frm = _tl_frm_set("tl-top-env", tl_new_cfunc(in, tl_cf_topenv), top_frm);
	top_frm = _tl_frm_set("tl-display", tl_new_cfunc(in, tl_cf_display), top_frm);
	top_frm = _tl_frm_set("tl-if", tl_new_cfunc(in, tl_cf_if), top_frm);

	top_frm = _tl_frm_set("tl-+", tl_new_cfunc(in, tl_cf_add), top_frm);
	top_frm = _tl_frm_set("tl--", tl_new_cfunc(in, tl_cf_sub), top_frm);
	top_frm = _tl_frm_set("tl-*", tl_new_cfunc(in, tl_cf_mul), top_frm);
	top_frm = _tl_frm_set("tl-/", tl_new_cfunc(in, tl_cf_div), top_frm);
	top_frm = _tl_frm_set("tl-%", tl_new_cfunc(in, tl_cf_mod), top_frm);

	top_frm = _tl_frm_set("tl-=", tl_new_cfunc(in, tl_cf_eq), top_frm);
	top_frm = _tl_frm_set("tl-<", tl_new_cfunc(in, tl_cf_less), top_frm);

	top_frm = _tl_frm_set("tl-nand", tl_new_cfunc(in, tl_cf_nand), top_frm);

	top_frm = _tl_frm_set("tl-cons", tl_new_cfunc(in, tl_cf_cons), top_frm);
	top_frm = _tl_frm_set("tl-car", tl_new_cfunc(in, tl_cf_car), top_frm);
	top_frm = _tl_frm_set("tl-cdr", tl_new_cfunc(in, tl_cf_cdr), top_frm);
	top_frm = _tl_frm_set("tl-type", tl_new_cfunc(in, tl_cf_type), top_frm);
	top_frm = _tl_frm_set("tl-null?", tl_new_cfunc(in, tl_cf_null), top_frm);

	top_frm = _tl_frm_set("tl-eval-in", tl_new_cfunc(in, tl_cf_evalin), top_frm);
	top_frm = _tl_frm_set("tl-apply", tl_new_cfunc(in, tl_cf_apply), top_frm);

	top_frm = _tl_frm_set("tl-read", tl_new_cfunc(in, tl_read), top_frm);

#ifdef DEBUG
	top_frm = _tl_frm_set("tl-debug-print", tl_new_cfunc(in, tl_cf_debug_print), top_frm);
#endif

	in->top_env = tl_new_pair(in, top_frm, in->top_env);
	in->env = in->top_env;
}

void tl_interp_cleanup(tl_interp *in) {
	while(in->top_alloc) {
		//in->printf(in->udata, "Visit: %p\n", in->top_alloc);
		tl_free(in, in->top_alloc);
	}
}
