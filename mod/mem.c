#include <string.h>
#include <stdint.h>
#include "tinylisp.h"

#define fail(msg) tl_error_set(in, tl_new_pair(in, tl_new_sym(in, msg), args)); tl_cfunc_return(in, in->false_)

static tl_tag PTR_TAG;

TL_MOD_INIT(tl_interp *in, const char *fname) {
	PTR_TAG = tl_new_tag(in);
	tl_object *frm = NULL;
	frm = tl_interp_load_funcs(in, frm, TL_START_INIT_ENTS, TL_STOP_INIT_ENTS);
	tl_env_merge(in, tl_env_top_pair(in), frm);
	return 1;
}

TL_CFBV(ptr_new, "ptr-new") {
	if(!tl_is_int(tl_first(args))) {
		fail("ptr-new on non-addr");
	}
	tl_cfunc_return(in, tl_new_ptr(in, (void*)tl_first(args)->ival, NULL, PTR_TAG));
}

TL_CFBV(ptr_of, "ptr-of") {
	tl_cfunc_return(in, tl_new_ptr(in, tl_first(args), NULL, PTR_TAG));
}

TL_CFBV(tag_of, "tag-of") {
	if(!tl_is_ptr(tl_first(args))) tl_cfunc_return(in, in->false_);
	tl_cfunc_return(in, tl_new_int(in, tl_first(args)->tag));
}

TL_CFBV(ptr_addr, "ptr-addr") {
	if(!tl_is_tag(tl_first(args), PTR_TAG)) {
		fail("addr of non-ptr");
	}
	tl_cfunc_return(in, tl_new_int(in, (long)tl_first(args)->ptr));
}

TL_CFBV(ptr_null, "ptr-null?") {
	if(!tl_is_tag(tl_first(args), PTR_TAG)) {
		fail("null? on non-ptr");
	}
	tl_cfunc_return(in, tl_first(args)->ptr ? in->false_ : in->true_);
}

TL_CFBV(ptr_read, "ptr-read") {
	tl_object *ptr = tl_first(args);
	tl_object *amt = tl_first(tl_next(args));
	if(!(tl_is_tag(ptr, PTR_TAG) && tl_is_int(amt))) {
		fail("invalid types (wanted ptr, int)");
	}
	tl_cfunc_return(in, tl_new_sym_data(in, ptr->ptr, amt->ival));
}

TL_CFBV(ptr_write, "ptr-write!") {
	tl_object *ptr = tl_first(args);
	tl_object *data = tl_first(tl_next(args));
	if(!(tl_is_tag(ptr, PTR_TAG) && tl_is_sym(data))) {
		fail("invalid types (wanted ptr, sym)");
	}
	memcpy(ptr->ptr, data->nm->here.data, data->nm->here.len);
	tl_cfunc_return(in, in->true_);
}

TL_CFBV(ptr_offset, "ptr-offset") {
	tl_object *ptr = tl_first(args);
	tl_object *amt = tl_first(tl_next(args));
	if(!(tl_is_tag(ptr, PTR_TAG) && tl_is_int(amt))) {
		fail("invalid types (wanted ptr, int)");
	}
	tl_cfunc_return(in, tl_new_ptr(in, (uint8_t*)ptr->ptr + amt->ival, NULL, PTR_TAG));
}
