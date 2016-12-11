#include "tinylisp.h"

#define _tl_frm_set(sm, obj, fm) tl_new_pair(tl_new_pair(tl_new_sym(sm), obj), fm)
void tl_interp_init(tl_interp *in) {
	in->true_ = tl_new_sym("#t");
	in->false_ = tl_new_sym("#f");
	in->error = NULL;

	in->top_env = TL_EMPTY_LIST;

	tl_object *top_frm = TL_EMPTY_LIST;
	top_frm = _tl_frm_set("#t", in->true_, top_frm);
	top_frm = _tl_frm_set("#f", in->false_, top_frm);

	top_frm = _tl_frm_set("error", tl_new_cfunc(tl_cf_error), top_frm);
	top_frm = _tl_frm_set("lambda", tl_new_cfunc(tl_cf_lambda), top_frm);
	top_frm = _tl_frm_set("macro", tl_new_cfunc(tl_cf_macro), top_frm);
	top_frm = _tl_frm_set("define", tl_new_cfunc(tl_cf_define), top_frm);
	top_frm = _tl_frm_set("set!", tl_new_cfunc(tl_cf_set), top_frm);
	top_frm = _tl_frm_set("env", tl_new_cfunc(tl_cf_env), top_frm);
	top_frm = _tl_frm_set("set-env!", tl_new_cfunc(tl_cf_setenv), top_frm);
	top_frm = _tl_frm_set("top-env", tl_new_cfunc(tl_cf_topenv), top_frm);
	top_frm = _tl_frm_set("display", tl_new_cfunc(tl_cf_display), top_frm);
	top_frm = _tl_frm_set("if", tl_new_cfunc(tl_cf_if), top_frm);

	top_frm = _tl_frm_set("+", tl_new_cfunc(tl_cf_add), top_frm);
	top_frm = _tl_frm_set("-", tl_new_cfunc(tl_cf_sub), top_frm);
	top_frm = _tl_frm_set("*", tl_new_cfunc(tl_cf_mul), top_frm);
	top_frm = _tl_frm_set("/", tl_new_cfunc(tl_cf_div), top_frm);
	top_frm = _tl_frm_set("%", tl_new_cfunc(tl_cf_mod), top_frm);

	top_frm = _tl_frm_set("=", tl_new_cfunc(tl_cf_eq), top_frm);
	top_frm = _tl_frm_set("<", tl_new_cfunc(tl_cf_less), top_frm);

	top_frm = _tl_frm_set("nand", tl_new_cfunc(tl_cf_nand), top_frm);

	top_frm = _tl_frm_set("cons", tl_new_cfunc(tl_cf_cons), top_frm);
	top_frm = _tl_frm_set("car", tl_new_cfunc(tl_cf_car), top_frm);
	top_frm = _tl_frm_set("cdr", tl_new_cfunc(tl_cf_cdr), top_frm);
	top_frm = _tl_frm_set("null?", tl_new_cfunc(tl_cf_null), top_frm);

	top_frm = _tl_frm_set("eval-in", tl_new_cfunc(tl_cf_evalin), top_frm);
	top_frm = _tl_frm_set("apply", tl_new_cfunc(tl_cf_apply), top_frm);

	top_frm = _tl_frm_set("read", tl_new_cfunc(tl_read), top_frm);

#ifdef DEBUG
	top_frm = _tl_frm_set("debug-print", tl_new_cfunc(tl_cf_debug_print), top_frm);
#endif

	in->top_env = tl_new_pair(top_frm, in->top_env);
	in->env = in->top_env;
}

void tl_interp_cleanup(tl_interp *in) {
	tl_free(in->top_env);
	tl_free(in->true_);
	tl_free(in->false_);
}
