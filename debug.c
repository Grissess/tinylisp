#ifdef DEBUG
#include <stdio.h>

#include "tinylisp.h"

void _indent(int lev) {
	int i;
	for(i = 0; i < lev; i++) {
		fprintf(stderr, "  ");
	}
}

void tl_dbg_print(tl_object *obj, int level) {
	_indent(level);
	if(!obj) {
		fprintf(stderr, "() (NULL object)\n");
		return;
	}
	switch(obj->kind) {
		case TL_INT:
			fprintf(stderr, "INT: %ld\n", obj->ival);
			break;

		case TL_SYM:
			fprintf(stderr, "SYM: len=%lu: ", obj->nm->here.len);
			fwrite(obj->nm->here.data, obj->nm->here.len, 1, stderr);
			fputc('\n', stderr);
			break;

		case TL_PAIR:
			fprintf(stderr, "PAIR:\n");
			_indent(level + 1);
			fprintf(stderr, "first:\n");
			tl_dbg_print(obj->first, level + 2);
			_indent(level + 1);
			fprintf(stderr, "next:\n");
			tl_dbg_print(obj->next, level + 2);
			break;

		case TL_CFUNC:
		case TL_CFUNC_BYVAL:
		case TL_THEN:
			fprintf(stderr, "%s: %p\n", obj->kind == TL_CFUNC ? "CFUNC" : (obj->kind == TL_CFUNC_BYVAL? "CFUNC_BYVAL" : "THEN"), obj->cfunc);
			_indent(level + 1);
			fprintf(stderr, "state:\n");
			tl_dbg_print(obj->state, level + 2);
			break;

		case TL_MACRO:
		case TL_FUNC:
			fprintf(stderr, "%s:\n", obj->kind == TL_MACRO ? "MACRO" : "FUNC");
			_indent(level + 1);
			fprintf(stderr, "args:\n");
			tl_dbg_print(obj->args, level + 2);
			if(obj->kind == TL_MACRO) {
				_indent(level + 1);
				fprintf(stderr, "envn: %s\n", obj->envn);
			}
			_indent(level + 1);
			fprintf(stderr, "body:\n");
			tl_dbg_print(obj->body, level + 2);
			/*
			_indent(level + 1);
			fprintf(stderr, "env:\n");
			tl_dbg_print(obj->env, level + 2);
			*/
			break;

		case TL_CONT:
			fprintf(stderr, "CONTINUATION:\n");
			_indent(level + 1);
			fprintf(stderr, "ret_conts:\n");
			tl_dbg_print(obj->ret_conts, level + 2);
			_indent(level + 1);
			fprintf(stderr, "ret_values:\n");
			tl_dbg_print(obj->ret_values, level + 2);
			/*
			_indent(level + 1);
			fprintf(stderr, "ret_env:\n");
			tl_dbg_print(obj->ret_env, level + 2);
			*/
			break;

		default:
			fprintf(stderr, "!!! UNKNOWN OBJECT KIND %d\n", obj->kind);
			break;
	}
}

static void _tl_cf_debug_print_k(tl_interp *in, tl_object *result, tl_object *_) {
	fprintf(stderr, "VALUE:\n");
	tl_dbg_print(tl_first(result), 0);
	tl_cfunc_return(in, in->true_);
}

TL_CF(debug_print, "debug-print") {
	fprintf(stderr, "EXPR:\n");
	tl_dbg_print(tl_first(args), 0);
	tl_eval_and_then(in, tl_first(args), NULL, _tl_cf_debug_print_k);
}
#endif
