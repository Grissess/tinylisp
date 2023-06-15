#include <stdio.h>
#include <errno.h>
#include "tinylisp.h"

static void _free_filep(tl_interp *in, tl_object *ptr) {
	fclose(ptr->ptr);
}

static tl_tag FILE_TAG;

TL_MOD_INIT(tl_interp *in, const char *fname) {
	FILE_TAG = tl_new_tag(in);
	tl_object *frm = NULL;
	frm = tl_interp_load_funcs(in, frm, TL_START_INIT_ENTS, TL_STOP_INIT_ENTS);
	tl_env_merge(in, tl_env_top_pair(in), frm);
	return 1;
}

TL_CF(io_errno, "io-errno") {
	tl_cfunc_return(in, tl_new_int(in, errno));
}

TL_CFBV(io_open, "io-open") {
	char *fname = tl_sym_to_cstr(in, tl_first(args));
	char *mode = tl_sym_to_cstr(in, tl_first(tl_next(args)));
	if((!fname) || (!mode)) goto fail;
	FILE *fp = fopen(fname, mode);
	if(!fp) goto fail;
	tl_cfunc_return(in, tl_new_ptr(in, fp, _free_filep, FILE_TAG));
	fail:
	tl_alloc_free(in, fname);
	tl_alloc_free(in, mode);
	tl_cfunc_return(in, in->false_);
}

TL_CFBV(io_close, "io-close") {
	tl_object *fobj = tl_first(args);
	if(!tl_is_tag(fobj, FILE_TAG)) tl_cfunc_return(in, in->false_);
	if(!fobj->ptr) tl_cfunc_return(in, in->false_);
	fclose(fobj->ptr);
	fobj->ptr = NULL;
	fobj->gcfunc = NULL;
	tl_cfunc_return(in, in->true_);
}

TL_CFBV(io_read, "io-read") {
	tl_object *fobj = tl_first(args), *bytes = tl_first(tl_next(args));
	if(!tl_is_tag(fobj, FILE_TAG) || !fobj->ptr || !tl_is_int(bytes)) tl_cfunc_return(in, in->false_);
	char *buffer = tl_alloc_malloc(in, bytes->ival);
	if(!buffer) tl_cfunc_return(in, in->false_);
	fread(buffer, 1, bytes->ival, fobj->ptr);
	tl_object *ret = tl_new_sym_data(in, buffer, bytes->ival);
	tl_alloc_free(in, buffer);
	tl_cfunc_return(in, ret);
}

TL_CFBV(io_write, "io-write") {
	tl_object *fobj = tl_first(args), *rest = tl_next(args);
	size_t bytes = 0;
	if(!tl_is_tag(fobj, FILE_TAG) || !fobj->ptr) tl_cfunc_return(in, in->false_);
	for(tl_list_iter(rest, sym)) {
		if(!tl_is_sym(sym)) continue;
		bytes += fwrite(sym->nm->here.data, 1, sym->nm->here.len, fobj->ptr);
	}
	tl_cfunc_return(in, tl_new_int(in, bytes));
}
