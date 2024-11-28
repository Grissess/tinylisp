#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "tinylisp.h"

#define QUOTED_SYM_CHARS "0123456789.,'\"` \n\r\t\b\v"

tl_object *_tl_print(tl_interp *, tl_object *, size_t);

static void _indent(tl_interp *in, size_t level) {
	while(level--) tl_putc(in, in->disp_indent);
}

void _print_pairs(tl_interp *in, tl_object *cur, size_t level) {
	while(cur) {
		if(in->disp_indent) {
			tl_putc(in, '\n');
			_indent(in, level);
		}
		if(!tl_is_pair(cur)) {
			tl_printf(in, ". ");
			_tl_print(in, cur, level);
			cur = TL_EMPTY_LIST;
		} else {
			_tl_print(in, tl_first(cur), level);
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
	return _tl_print(in, obj, 0);
}

tl_object *_tl_print(tl_interp *in, tl_object *obj, size_t level) {
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
			if(obj->nm->here.len == 0 || _mempbrk(obj->nm->here.data, QUOTED_SYM_CHARS, obj->nm->here.len)) {
				tl_putc(in, '"');
				tl_write(in, obj->nm->here.data, obj->nm->here.len);
				tl_putc(in, '"');
			} else {
				tl_write(in, obj->nm->here.data, obj->nm->here.len);
			}
			break;

		case TL_PAIR:
			tl_printf(in, "(");
			_print_pairs(in, obj, level + 1);
			if(in->disp_indent) {
				tl_putc(in, '\n');
				_indent(in, level);
			}
			tl_printf(in, ")");
			break;

		case TL_CFUNC:
		case TL_CFUNC_BYVAL:
		case TL_THEN:
			tl_printf(in, "%s:%p", obj->name ? obj->name : (obj->kind == TL_CFUNC ? "<cfunc>" : (obj->kind == TL_CFUNC_BYVAL ? "<cfunc_byval>" : "<then>")), obj->cfunc);
			/*if(obj->state) {
				tl_printf(in, ":(");
				tl_print(in, obj->state);
				tl_printf(in, ")");
			}*/
			break;

		case TL_MACRO:
		case TL_FUNC:
			tl_printf(in, "(%s ", obj->kind == TL_MACRO ? "macro" : "lambda");
			_tl_print(in, obj->args, level + 1);
			tl_putc(in, ' ');
			if(tl_is_macro(obj)) {
				_tl_print(in, obj->envn, level + 1);
				tl_putc(in, ' ');
			}
			_print_pairs(in, obj->body, level + 1);
			if(in->disp_indent) {
				tl_putc(in, '\n');
				_indent(in, level);
			}
			tl_printf(in, ")");
			break;

		case TL_CONT:
			tl_printf(in, "cont:%p", obj);
			break;

		case TL_PTR:
			tl_printf(in, "ptr:%p", obj->ptr);
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
	union {
		const char *s;
		tl_buffer *b;
	} temp;
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
					temp.s = va_arg(ap, const char *);
					if(!temp.s)
						tl_puts(in, "<NULL>");
					else
						tl_puts(in, temp.s);
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

				case 'z':  /* FIXME: zx as above */
					snprintf(buf, 32, "%zx", va_arg(ap, size_t));
					tl_puts(in, buf);
					cur += 2;
					break;

				case 'd':
					snprintf(buf, 32, "%d", va_arg(ap, int));
					tl_puts(in, buf);
					cur++;
					break;

				case 'N':  /* non-standard */
					temp.b = va_arg(ap, tl_buffer *);
					snprintf(buf, 32, "%ld", temp.b->len);
					tl_puts(in, buf);
					tl_putc(in, ':');
					tl_write(in, temp.b->data, temp.b->len);
					cur++;
					break;

				case 'O':  /* non-standard */
					tl_print(in, va_arg(ap, tl_object *));
					cur++;
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
