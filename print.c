#include <string.h>

#include "tinylisp.h"

#define QUOTED_SYM_CHARS "0123456789.,'\"`"

void _print_pairs(tl_interp *in, tl_object *cur) {
	while(cur) {
		if(!tl_is_pair(cur)) {
			in->printf(in->udata, ". ");
			tl_print(in, cur);
			cur = NULL;
		} else {
			tl_print(in, tl_first(cur));
			if(tl_next(cur)) {
				in->printf(in->udata, " ");
			}
			cur = tl_next(cur);
		}
	}
}

tl_object *tl_print(tl_interp *in, tl_object *obj) {
	tl_object *cur;
	if(!obj) {
		in->printf(in->udata, "()");
		return in->true_;
	}
	switch(obj->kind) {
		case TL_INT:
			in->printf(in->udata, "%ld", obj->ival);
			break;

		case TL_SYM:
			if(strpbrk(obj->str, QUOTED_SYM_CHARS) || strlen(obj->str) == 0) {
				in->printf(in->udata, "\"%s\"", obj->str);
			} else {
				in->printf(in->udata, "%s", obj->str);
			}
			break;

		case TL_PAIR:
			in->printf(in->udata, "(");
			_print_pairs(in, obj);
			in->printf(in->udata, ")");
			break;

		case TL_CFUNC:
		case TL_CFUNC_BYVAL:
		case TL_THEN:
			in->printf(in->udata, "%s:%p", obj->name ? obj->name : (obj->kind == TL_CFUNC ? "<cfunc>" : (obj->kind == TL_CFUNC_BYVAL ? "<cfunc_byval>" : "<then>")), obj->cfunc);
			break;

		case TL_MACRO:
		case TL_FUNC:
			in->printf(in->udata, "(%s ", obj->kind == TL_MACRO ? "macro" : "lambda");
			tl_print(in, obj->args);
			in->printf(in->udata, " ");
			if(tl_is_macro(obj)) {
				in->printf(in->udata, "%s ", obj->envn);
			}
			_print_pairs(in, obj->body);
			in->printf(in->udata, ")");
			break;

		case TL_CONT:
			in->printf(in->udata, "cont:%p", obj);
			break;

		default:
			in->printf(in->udata, "<unknown object kind %d>", obj->kind);
			break;
	}
	return in->true_;
}
