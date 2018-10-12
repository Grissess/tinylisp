#include <stdlib.h>
#include <ctype.h>

#include "tinylisp.h"

#ifndef MAX_SYM_LEN
#define MAX_SYM_LEN 64
#endif

#define return_sym_from_cstr(in, s) do { \
	tl_object *ret = tl_new_sym((in), (s)); \
	free((s)); \
	return ret; \
} while(0)

/* FIXME: NULL and TL_EMPTY_LIST are the same; empty list can signal EOF */
tl_object *tl_read(tl_interp *in, tl_object *args) {
	int c, d, q, idx = 0;
	long ival = 0;
	tl_object *list = TL_EMPTY_LIST;
	char *symbuf;

	while(1) {
		switch(c = tl_getc(in)) {
			case EOF:
				return NULL;
				break;

			case ' ': case '\n': case '\t': case '\v': case '\r': case '\b':
				continue;
				break;

			case ';':
				while(tl_getc(in) != '\n');
				continue;
				break;

			case '(':
				while(1) {
					switch(d = tl_getc(in)) {
						case ' ': case '\n': case '\t': case '\v': case '\r': case '\b':
							continue;
							break;

						case ')':
							return tl_list_rvs(in, list);
							break;

						default:
							tl_putback(in, d);
							list = tl_new_pair(in, tl_read(in, TL_EMPTY_LIST), list);
							break;
					}
				}
				break;

			case '"':
				q = c;
				symbuf = calloc(MAX_SYM_LEN + 1, sizeof(char));
				while(idx < MAX_SYM_LEN && (d = in->readf(in->udata, in)) != q) {
					symbuf[idx++] = d;
				}
				return_sym_from_cstr(in, symbuf);
				break;

			default:
				if(isdigit(c)) {
					ival = c - '0';
					while(isdigit((c = tl_getc(in)))) {
						ival *= 10;
						ival += c - '0';
					}
					tl_putback(in, c);
					return tl_new_int(in, ival);
				}
				for(tl_list_iter(in->prefixes, kv)) {
					tl_object *key = tl_first(kv);
					tl_object *val = tl_next(kv);
					if(key && val && tl_is_sym(key) && key->str[0] == c) {
						list = tl_read(in, TL_EMPTY_LIST);
						return tl_new_pair(in, val, tl_new_pair(in, list, TL_EMPTY_LIST));
					}
				}
				symbuf = calloc(MAX_SYM_LEN + 1, sizeof(char));
				symbuf[idx++] = c;
				while(idx < MAX_SYM_LEN) {
					switch(d = tl_getc(in)) {
						case ' ': case '\n': case '\t': case '\v': case '\r': case '\b':
							return_sym_from_cstr(in, symbuf);
							break;

						case '(': case ')':
							tl_putback(in, d);
							return_sym_from_cstr(in, symbuf);
							break;

						default:
							symbuf[idx++] = d;
							break;
					}
				}
				return_sym_from_cstr(in, symbuf);
				break;
		}
	}
}
