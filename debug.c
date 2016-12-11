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
			fprintf(stderr, "SYM: %s\n", obj->str);
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
			fprintf(stderr, "CFUNC: %p\n", obj->cfunc);
			break;

		case TL_FUNC:
		case TL_MACRO:
			if(obj->kind == TL_FUNC) {
				fprintf(stderr, "FUNC:\n");
			} else {
				fprintf(stderr, "MACRO:\n");
			}
			_indent(level + 1);
			fprintf(stderr, "args:\n");
			tl_dbg_print(obj->args, level + 2);
			_indent(level + 1);
			fprintf(stderr, "body:\n");
			tl_dbg_print(obj->body, level + 2);
			_indent(level + 1);
			fprintf(stderr, "env:\n");
			tl_dbg_print(obj->env, level + 2);
			break;

		default:
			fprintf(stderr, "!!! UNKNOWN OBJECT KIND %d\n", obj->kind);
			break;
	}
}

tl_object *tl_cf_debug_print(tl_interp *in, tl_object *obj) {
	fprintf(stderr, "EXPR:\n");
	tl_dbg_print(tl_first(obj), 0);
	obj = tl_eval(in, tl_first(obj));
	fprintf(stderr, "VALUE:\n");
	tl_dbg_print(obj, 0);
	return in->true_;
}
#endif
