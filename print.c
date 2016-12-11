#include <string.h>

#include "tinylisp.h"

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
			if(strchr(obj->str, ' ') || strlen(obj->str) == 0) {
				in->printf(in->udata, "\"%s\"", obj->str);
			} else {
				in->printf(in->udata, "%s", obj->str);
			}
			break;

		case TL_PAIR:
			in->printf(in->udata, "(");
			cur = obj;
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
			in->printf(in->udata, ")");
			break;

		case TL_CFUNC:
			in->printf(in->udata, "cfunc:%p", obj->cfunc);
			break;

		case TL_FUNC:
		case TL_MACRO:
			if(tl_is_macro(obj)) {
				in->printf(in->udata, "(macro ");
			} else {
				in->printf(in->udata, "(lambda ");
			}
			tl_print(in, obj->args);
			in->printf(in->udata, " ");
			cur = obj->body;
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
			in->printf(in->udata, ")");
			break;

		default:
			in->printf(in->udata, "<unknown object kind %d>", obj->kind);
			break;
	}
	return in->true_;
}
