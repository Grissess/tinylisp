#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "tinylisp.h"

#define QUOTED_SYM_CHARS "0123456789.,'\"`"

void _print_pairs(tl_interp *in, tl_object *cur) {
	while(cur) {
		if(!tl_is_pair(cur)) {
			tl_printf(in, ". ");
			tl_print(in, cur);
			cur = NULL;
		} else {
			tl_print(in, tl_first(cur));
			if(tl_next(cur)) {
				tl_printf(in, " ");
			}
			cur = tl_next(cur);
		}
	}
}

char *_mempbrk(char *m, char *s, size_t sz) {
	size_t i;
	char *t;

	for(i = 0; i < sz; i++, m++)
		for(t = s; *t; t++)
			if(*m == *t)
				return m;
	return NULL;
}

tl_object *tl_print(tl_interp *in, tl_object *obj) {
	tl_object *cur;
	if(!obj) {
		tl_printf(in, "()");
		return in->true_;
	}
	switch(obj->kind) {
		case TL_INT:
			tl_printf(in, "%ld", obj->ival);
			break;

		case TL_SYM:
			if(obj->len == 0 || _mempbrk(obj->str, QUOTED_SYM_CHARS, obj->len)) {
				tl_putc(in, '"');
				tl_write(in, obj->str, obj->len);
				tl_putc(in, '"');
			} else {
				tl_write(in, obj->str, obj->len);
			}
			break;

		case TL_PAIR:
			tl_printf(in, "(");
			_print_pairs(in, obj);
			tl_printf(in, ")");
			break;

		case TL_CFUNC:
		case TL_CFUNC_BYVAL:
		case TL_THEN:
			tl_printf(in, "%s:%p", obj->name ? obj->name : (obj->kind == TL_CFUNC ? "<cfunc>" : (obj->kind == TL_CFUNC_BYVAL ? "<cfunc_byval>" : "<then>")), obj->cfunc);
			break;

		case TL_MACRO:
		case TL_FUNC:
			tl_printf(in, "(%s ", obj->kind == TL_MACRO ? "macro" : "lambda");
			tl_print(in, obj->args);
			tl_printf(in, " ");
			if(tl_is_macro(obj)) {
				tl_printf(in, "%s ", obj->envn);
			}
			_print_pairs(in, obj->body);
			tl_printf(in, ")");
			break;

		case TL_CONT:
			tl_printf(in, "cont:%p", obj);
			break;

		default:
			tl_printf(in, "<unknown object kind %d>", obj->kind);
			break;
	}
	return in->true_;
}

void tl_puts(tl_interp *in, const char *s) {
	while(*s != 0) tl_putc(in, *s++);
}

void tl_write(tl_interp *in, const char *data, size_t len) {
	size_t i = 0;
	while(i++ < len) tl_putc(in, *data++);
}

void tl_printf(tl_interp *in, const char *cur, ...) {
	va_list ap;
	const char *s;
	char buf[32];

	va_start(ap, cur);
	while(*cur != 0) {
		if(*cur == '%') {
			cur++;
			switch(*cur) {
				case 0:
					tl_putc(in, '%');
					break;

				case '%':
					tl_putc(in, '%');
					cur++;
					break;

				case 's':
					s = va_arg(ap, const char *);
					if(!s)
						tl_puts(in, "<NULL>");
					else
						tl_puts(in, s);
					cur++;
					break;

				case 'p':
					snprintf(buf, 32, "%p", va_arg(ap, void *));
					tl_puts(in, buf);
					cur++;
					break;

				case 'l':  /* FIXME: assuming ld throughout source */
					snprintf(buf, 32, "%ld", va_arg(ap, long));
					tl_puts(in, buf);
					cur += 2;
					break;

				default:
					tl_putc(in, '%');
					tl_putc(in, *cur++);
					break;
			}
		} else {
			tl_putc(in, *cur++);
		}
	}

	va_end(ap);
}
