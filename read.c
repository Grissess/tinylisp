#include <stdlib.h>
#include <ctype.h>

#include "tinylisp.h"

#ifndef MAX_SYM_LEN
#define MAX_SYM_LEN 64
#endif

/* FIXME: NULL and TL_EMPTY_LIST are the same; empty list can signal EOF */
tl_object *tl_read(tl_interp *in, tl_object *args) {
	int c, d, q, idx = 0;
	long ival = 0;
	tl_object *list = TL_EMPTY_LIST;
	char *symbuf = calloc(MAX_SYM_LEN + 1, sizeof(char));

	while(1) {
		switch(c = in->readf(in->udata)) {
			case EOF:
				return NULL;
				break;

			case ' ': case '\n': case '\t': case '\v': case '\r': case '\b':
				continue;
				break;

			case ';':
				while(in->readf(in->udata) != '\n');
				continue;
				break;

			case '(':
				while(1) {
					switch(d = in->readf(in->udata)) {
						case ' ': case '\n': case '\t': case '\v': case '\r': case '\b':
							continue;
							break;

						case ')':
							return tl_list_rvs(list);
							break;

						default:
							in->putbackf(in->udata, d);
							list = tl_new_pair(tl_read(in, TL_EMPTY_LIST), list);
							break;
					}
				}
				break;

			case '"':
				q = c;
				while(idx < MAX_SYM_LEN && (d = in->readf(in->udata)) != q) {
					symbuf[idx++] = d;
				}
				return tl_new_sym(symbuf);
				break;

			default:
				if(isdigit(c)) {
					ival = c - '0';
					while(isdigit((c = in->readf(in->udata)))) {
						ival *= 10;
						ival += c - '0';
					}
					in->putbackf(in->udata, c);
					return tl_new_int(ival);
				}
				for(tl_list_iter(in->prefixes, kv)) {
					tl_object *key = tl_first(kv);
					tl_object *val = tl_next(kv);
					if(key && val && tl_is_sym(key) && key->str[0] == c) {
						list = tl_read(in, TL_EMPTY_LIST);
						return tl_new_pair(val, tl_new_pair(list, TL_EMPTY_LIST));
					}
				}
				symbuf[idx++] = c;
				while(idx < MAX_SYM_LEN) {
					switch(d = in->readf(in->udata)) {
						case ' ': case '\n': case '\t': case '\v': case '\r': case '\b':
							return tl_new_sym(symbuf);
							break;

						case '(': case ')':
							in->putbackf(in->udata, d);
							return tl_new_sym(symbuf);
							break;

						default:
							symbuf[idx++] = d;
							break;
					}
				}
				return tl_new_sym(symbuf);
				break;
		}
	}
}
