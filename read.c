#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include "tinylisp.h"

#ifndef DEFAULT_SYM_LEN
/** The default size of a symbol buffer, in bytes.
 *
 * It's more time-efficient to keep this small, but allowing it to grow is more
 * space-efficient. The growth algorithm is exponential.
 */
#define DEFAULT_SYM_LEN 64
#endif

/** A helper macro to return a TL_SYM from a C string which was allocated using malloc(). */
#define return_sym_from_buf(in, s, sz) do { \
	tl_object *ret = tl_new_sym_data((in), (s), (sz)); \
	free((s)); \
	return ret; \
} while(0)

/** A helper macro to add another character to the buffer. */
#define add_to_cstr(in, buf, sz, idx, c) do { \
	buf[idx++] = c; \
	if(idx >= sz) { \
		sz <<= 1; \
		buf = tl_alloc_realloc(in, buf, sz); \
		assert(buf); \
	} \
} while(0)

#define top_is_char(in, args) do { \
	if(!tl_is_int(tl_first(args))) { \
		tl_error_set((in), tl_new_pair((in), tl_new_sym((in), "not a char"), tl_first(args))); \
		tl_cfunc_return((in), (in)->false_); \
	} \
} while(0)

#define reader_proto(x) static void _tl_read_##x##_k(tl_interp *, tl_object *, tl_object *)
reader_proto(top);
reader_proto(comment);
reader_proto(string);
reader_proto(list);
reader_proto(int);
reader_proto(sym);

#define reader(x) static void _tl_read_##x##_k(tl_interp *in, tl_object *args, tl_object *state)

#define reader_prologue(in, args) int ch; \
	top_is_char(in, args); \
	ch = (int)tl_first(args)->ival

/* isspace is useful, but we don't have match guards in C */
#define case_any_ws case ' ': case '\n': case '\t': case '\v': case '\r': case '\b'
#define is_any_ws(c) ((c) == ' ' || (c) == '\n' || (c) == '\t' || (c) == '\v' || (c) == '\r' || (c) == '\b')

#define make_read_buffer(in) do { \
	if(!(in)->read_buffer) { \
		(in)->read_ptr = 0; \
		(in)->read_sz = DEFAULT_SYM_LEN; \
		(in)->read_buffer = tl_alloc_malloc((in), (in)->read_sz); \
		assert((in)->read_buffer); \
	} \
} while(0)

/* FIXME: NULL and TL_EMPTY_LIST are the same; empty list can signal EOF */
/** Read a value, as a coroutine.
 *
 * The value is placed on the value stack. You probably want to `tl_push_apply`
 * to receive it after it is computed. See `tl_read_and_then` for a convenience
 * macro.
 *
 * The value is considered "direct", since the parser has no dichotomy of value
 * or syntax. In the typical case, the "direct" value received from this
 * routine is passed to something like `tl_eval_and_then`, which--via
 * `tl_push_eval`--considers any resulting applications to be syntactic.
 * However, it also means that `tl-read` from user code does what one expects.
 */
void tl_read(tl_interp *in) {
	tl_getc_and_then(in, TL_EMPTY_LIST, _tl_read_top_k);
}


static void _tl_read_top_prefix_k(tl_interp *in, tl_object *args, tl_object *state) {
	tl_cfunc_return(in, tl_new_pair(in, state, tl_new_pair(in, tl_first(args), TL_EMPTY_LIST)));
}

reader(top) {
	reader_prologue(in, args);
	switch(ch) {
		case EOF:
			tl_cfunc_return(in, TL_EMPTY_LIST);
			break;

		case_any_ws:
			tl_getc_and_then(in, state, _tl_read_top_k);
			break;

		case ';':
			tl_getc_and_then(in, state, _tl_read_comment_k);
			break;

		case '"':
			tl_getc_and_then(in, state, _tl_read_string_k);
			break;

		case '(':
			tl_getc_and_then(in, TL_EMPTY_LIST, _tl_read_list_k);
			break;

		default:
			if(isdigit(ch)) {
				tl_getc_and_then(in, tl_new_int(in, ch - '0'), _tl_read_int_k);
				return;
			}
			for(tl_list_iter(in->prefixes, kv)) {
				tl_object *k = tl_first(kv);
				tl_object *v = tl_next(kv);
				if(k && v && tl_is_sym(k) && k->nm->here.len > 0 && k->nm->here.data[0] == ch) {
					tl_push_apply(in, 1,
							tl_new_then(in, _tl_read_top_prefix_k, v, "_tl_read_top_k<prefix>"),
							in->env
					);
					tl_getc_and_then(in, TL_EMPTY_LIST, _tl_read_top_k);
					return;
				}
			}
			tl_putback(in, ch);
			tl_getc_and_then(in, state, _tl_read_sym_k);
			break;
	}
}

reader(comment) {
	reader_prologue(in, args);
	switch(ch) {
		case EOF:
			tl_cfunc_return(in, TL_EMPTY_LIST);
			break;

		case '\n':
			tl_getc_and_then(in, state, _tl_read_top_k);
			break;

		default:
			tl_getc_and_then(in, state, _tl_read_comment_k);
			break;
	}
}

reader(string) {
	tl_object *sym;
	reader_prologue(in, args);
	make_read_buffer(in);
	switch(ch) {
		case EOF:
			tl_error_set(in, tl_new_sym(in, "EOF in string"));
			tl_cfunc_return(in, in->false_);
			break;

		case '"':
			sym = tl_new_sym_data(in, in->read_buffer, in->read_ptr);
			tl_alloc_free(in, in->read_buffer);
			in->read_buffer = NULL;
			tl_cfunc_return(in, sym);
			break;

		default:
			add_to_cstr(in, in->read_buffer, in->read_sz, in->read_ptr, ch);
			tl_getc_and_then(in, state, _tl_read_string_k);
			break;
	}
}

/* Not technically a reader */
static void _tl_read_pair_cons_k(tl_interp *in, tl_object *args, tl_object *state) {
	state = tl_new_pair(in, tl_first(args), state);
	tl_getc_and_then(in, state, _tl_read_list_k);
}

reader(pair_improp_check) {
	reader_prologue(in, args);
	switch(ch) {
		case_any_ws:
			tl_getc_and_then(in, state, _tl_read_pair_improp_check_k);
			break;

		case ')':  /* The expected case */
			tl_cfunc_return(in, tl_list_rvs_improp(in, state));
			break;

		default:  /* TODO: error */
			tl_putback(in, ch);
			state = tl_new_pair(in, tl_first(state), tl_new_pair(in, tl_new_sym(in, "."), tl_next(state)));
			tl_getc_and_then(in, state, _tl_read_list_k);
	}
}

static void _tl_read_pair_improp_k(tl_interp *in, tl_object *args, tl_object *state) {
	state = tl_new_pair(in, tl_first(args), state);
	tl_getc_and_then(in, state, _tl_read_pair_improp_check_k);
}

reader(list) {
	reader_prologue(in, args);
	switch(ch) {
		case_any_ws:
			tl_getc_and_then(in, state, _tl_read_list_k);
			break;

		case ')':
			tl_cfunc_return(in, tl_list_rvs(in, state));
			break;

		case '.':
			tl_push_apply(in, 1,
					tl_new_then(in, _tl_read_pair_improp_k, state, "_tl_read_pair<improp>"),
					in->env
			);
			tl_getc_and_then(in, TL_EMPTY_LIST, _tl_read_top_k);
			break;

		default:
			tl_putback(in, ch);
			/* Catch the return of _tl_read_top_k into our list */
			tl_push_apply(in, 1,
					tl_new_then(in, _tl_read_pair_cons_k, state, "_tl_read_list_k<cons>"),
					in->env
			);
			tl_getc_and_then(in, TL_EMPTY_LIST, _tl_read_top_k);
			break;
	}
}

reader(int) {
	reader_prologue(in, args);
	if(isdigit(ch)) {
		state = tl_new_int(in, state->ival * 10 + (ch - '0'));
		tl_getc_and_then(in, state, _tl_read_int_k);
	} else {
		tl_putback(in, ch);
		tl_cfunc_return(in, state);
	}
}

reader(sym) {
	tl_object *sym;
	reader_prologue(in, args);
	make_read_buffer(in);
	switch(ch) {
		case '(': case ')':
			tl_putback(in, ch);
			/* Fall through */
		case_any_ws: case EOF:
			sym = tl_new_sym_data(in, in->read_buffer, in->read_ptr);
			tl_alloc_free(in, in->read_buffer);
			in->read_buffer = NULL;
			tl_cfunc_return(in, sym);
			break;

		default:
			add_to_cstr(in, in->read_buffer, in->read_sz, in->read_ptr, ch);
			tl_getc_and_then(in, state, _tl_read_sym_k);
			break;
	}
}

