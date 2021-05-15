#include <stdlib.h>
#include <ctype.h>

#include "tinylisp.h"

#ifndef MAX_SYM_LEN
/** The absolute maximum size of a symbol read can handle, in bytes. */
#define MAX_SYM_LEN 64
#endif

/** A helper macro to return a TL_SYM from a C string which was allocated using malloc(). */
#define return_sym_from_cstr(in, s) do { \
	tl_object *ret = tl_new_sym((in), (s)); \
	free((s)); \
	return ret; \
} while(0)

/* FIXME: NULL and TL_EMPTY_LIST are the same; empty list can signal EOF */
/** Read a value.
 *
 * This invokes `tl_getc` to get the next character, and may also invoke
 * `tl_putback` to make `tl_getc` return a different character on the next
 * invocation.
 *
 * This function is heavily recursive, and reading a large data structure very
 * well could cause issues with the C stack size on constrained platforms. This
 * is a known bug.
 *
 * The returned value is the entire expression read using `tl_getc` (normally
 * from wherever `readf` is reading data, which is, e.g., stdin in the
 * `main.c` REPL). If `readf` returns `EOF` immediately, this returns NULL.
 * Note that there is presently no way to discriminate between an empty list
 * (`TL_EMPTY_LIST`) of the syntax `()` and a `NULL` return due to EOF.
 */
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

						case '.':
							list = tl_new_pair(in, tl_read(in, TL_EMPTY_LIST), list);
							while(d = tl_getc(in)) {
								if(d != ' ' && d != '\n' && d != '\t' && d != '\v' && d != '\r' && d != 'b') break;
							}

							/* This definitely SHOULD be the case; the other path is errant. */
							if(d == ')') {
								return tl_list_rvs_improp(in, list);
							} else {
								tl_putback(in, d);
								list = tl_new_pair(in, tl_first(list), tl_new_pair(in, tl_new_sym(in, "."), tl_next(list)));
							}

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
				symbuf = tl_calloc(in, MAX_SYM_LEN + 1, sizeof(char));
				while(idx < MAX_SYM_LEN && (d = in->readf(in)) != q) {
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
					if(key && val && tl_is_sym(key) && key->nm->here.len > 0 && key->nm->here.data[0] == c) {
						list = tl_read(in, TL_EMPTY_LIST);
						return tl_new_pair(in, val, tl_new_pair(in, list, TL_EMPTY_LIST));
					}
				}
				symbuf = tl_calloc(in, MAX_SYM_LEN + 1, sizeof(char));
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
