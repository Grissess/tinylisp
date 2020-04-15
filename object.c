#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "tinylisp.h"

/** Create a new object.
 *
 * The object has undefined type, which must be initialized. The more specific
 * constructors already do this.
 *
 * The returned object is threaded into the garbage collector, and will be
 * collected if it is not reachable from any root on the next call to `tl_gc`.
 */
tl_object *tl_new(tl_interp *in) {
	tl_object *obj = tl_alloc_malloc(in, sizeof(tl_object));
	if(!obj) {
		tl_gc(in);
		obj = tl_alloc_malloc(in, sizeof(tl_object));
		assert(obj);
	}
	obj->next_alloc = in->top_alloc;
	obj->prev_alloc = NULL;
	if(in->top_alloc) in->top_alloc->prev_alloc = obj;
	in->top_alloc = obj;
	return obj;
}

/** Create a new integer object.
 *
 * Note that TL does not cache integers; every integer object is a uniqe
 * pointer. If you find yourself frequently using the same value, you may wish
 * to implement your own cache.
 */
tl_object *tl_new_int(tl_interp *in, long ival) {
	tl_object *obj = tl_new(in);
	obj->kind = TL_INT;
	obj->ival = ival;
	return obj;
}

/** Create a new symbol object.
 *
 * This assumes that `str` is a C string (terminated by a NULL byte). Use
 * `tl_new_sym_data` for more general-purpose data.
 */
tl_object *tl_new_sym(tl_interp *in, const char *str) {
	if(str) {
		return tl_new_sym_data(in, str, strlen(str));
	} else {
		return tl_new_sym_data(in, NULL, 0);
	}
}

/** Creates a new symbol object with length.
 *
 * TL assumes that the data in this symbol is immutable, especially for
 * comparisons. Applications shouldn't mutate it, but are free to make as many
 * symbols as they would like.
 */
tl_object *tl_new_sym_data(tl_interp *in, const char *data, size_t len) {
	tl_object *obj = tl_new(in);
	char *copy = NULL;
	if(len > 0) {
		copy = tl_alloc_malloc(in, len);
		memcpy(copy, data, len);
	}
	obj->kind = TL_SYM;
	obj->str = copy;
	obj->len = len;
	obj->hash = tl_data_hash(copy, len);
	return obj;
}

/** Creates a new pair.
 *
 * This is the underlying constructor for a "cons" cell.
 */
tl_object *tl_new_pair(tl_interp *in, tl_object *first, tl_object *next) {
	tl_object *obj = tl_new(in);
	obj->kind = TL_PAIR;
	obj->first = first;
	obj->next = next;
	return obj;
}

/** Creates a new continuation ("then").
 *
 * These are the constituents of the continuation stack (`interp->conts`). See
 * `eval.c` for details.
 */
tl_object *tl_new_then(tl_interp *in, void (*cfunc)(tl_interp *, tl_object *, tl_object *), tl_object *state, const char *name) {
	tl_object *obj = tl_new(in);
	obj->kind = TL_THEN;
	obj->cfunc = cfunc;
	obj->state = state;
	obj->name = name ? tl_strdup(in, name) : NULL;
	return obj;
}

/** Creates a new cfunction.
 *
 * These are typically used to expose C code to TinyLISP code. A base C
 * function receives its arguments syntactically (unevaluated, as does a
 * macro). For C functions which expect all arguments by value, it's usually
 * better to use `tl_new_cfunc_byval` for many reasons.
 */
tl_object *_tl_new_cfunc(tl_interp *in, void (*cfunc)(tl_interp *, tl_object *, tl_object *), const char *name) {
	tl_object *obj = tl_new_then(in, cfunc, TL_EMPTY_LIST, name);
	obj->kind = TL_CFUNC;
	return obj;
}

/** Creates a new "by-value" C function.
 *
 * Unlike a base C function, this version receives all of its arguments by
 * value, which is what most functions expect (and the most flexible), as does
 * a TinyLISP lambda.
 */
tl_object *_tl_new_cfunc_byval(tl_interp *in, void (*cfunc)(tl_interp *, tl_object *, tl_object *), const char *name) {
	tl_object *obj = tl_new_then(in, cfunc, TL_EMPTY_LIST, name);
	obj->kind = TL_CFUNC_BYVAL;
	return obj;
}

/** Creates a new macro.
 *
 * This is rarely needed from C (except for tl-macro).
 */
tl_object *tl_new_macro(tl_interp *in, tl_object *args, tl_object *envn, tl_object *body, tl_object *env) {
	tl_object *obj = tl_new(in);
	obj->kind = envn ? TL_MACRO : TL_FUNC;
	obj->args = args;
	obj->body = body;
	obj->env = env;
	obj->envn = envn;
	return obj;
}

/** Creates a new continuation.
 *
 * These are the objects created by call-with-current-continuation (call/cc).
 */
tl_object *tl_new_cont(tl_interp *in, tl_object *env, tl_object *conts, tl_object *values) {
	tl_object *obj = tl_new(in);
	obj->kind = TL_CONT;
	obj->ret_env = env;
	obj->ret_conts = conts;
	obj->ret_values = values;
	return obj;
}

/** Free an object.
 *
 * TinyLISP has a tracing GC, so, as a rule, you should never need to do this.
 * However, you may do this if you can prove that you are removing the last
 * reference to this object, as a slight optimization. The amount of
 * optimization is rarely worth the proof (since TL code can easily capture
 * references you don't know about in many cases), so it's best to simply not
 * use this function (except from within `tl_gc`).
 */
void tl_free(tl_interp *in, tl_object *obj) {
	if(!obj) return;
	if(obj->prev_alloc) {
		obj->prev_alloc->next_alloc = tl_make_next_alloc(
			obj->prev_alloc->next_alloc,
			tl_next_alloc(obj)
		);
	} else {
		in->top_alloc = tl_make_next_alloc(
			in->top_alloc,
			tl_next_alloc(obj)
		);
	}
	if(tl_next_alloc(obj)) {
		tl_next_alloc(obj)->prev_alloc = obj->prev_alloc;
	}
	switch(obj->kind) {
		case TL_SYM:
			tl_alloc_free(in, obj->str);
			break;

		case TL_CFUNC:
		case TL_CFUNC_BYVAL:
		case TL_THEN:
			tl_alloc_free(in, obj->name);
			break;

		default:
			break;
	}
	tl_alloc_free(in, obj);
}

/** Mark an object and its descendents.
 *
 * When adding a new type of function, be sure that this recursively visits the
 * new object type's descendent `tl_object` pointers.
 */
static void _tl_mark_pass(tl_object *obj) {
	if(!obj) return;
	if(tl_is_marked(obj)) return;
	tl_mark(obj);
	switch(obj->kind) {
		case TL_INT:
		case TL_SYM:
			break;

		case TL_CFUNC:
		case TL_CFUNC_BYVAL:
		case TL_THEN:
			_tl_mark_pass(obj->state);
			break;

		case TL_FUNC:
		case TL_MACRO:
			_tl_mark_pass(obj->args);
			_tl_mark_pass(obj->body);
			_tl_mark_pass(obj->env);
			_tl_mark_pass(obj->envn);
			break;

		case TL_PAIR:
			_tl_mark_pass(obj->first);
			_tl_mark_pass(obj->next);
			break;

		case TL_CONT:
			_tl_mark_pass(obj->ret_env);
			_tl_mark_pass(obj->ret_conts);
			_tl_mark_pass(obj->ret_values);
			break;

		default:
			assert(0);
	}
}

/** Perform a garbage collection pass.
 *
 * This calls `tl_free` on objects registered to the garbage collector (via
 * `tl_new` and related) but which are no longer reachable from the roots
 * (generally, all `tl_object *` in the interpreter).
 */
void tl_gc(tl_interp *in) {
	tl_object *obj = in->top_alloc;
	tl_object *tmp;
	while(obj) {
		tl_unmark(obj);
		obj = tl_next_alloc(obj);
	}
	_tl_mark_pass(in->true_);
	_tl_mark_pass(in->false_);
	_tl_mark_pass(in->error);
	_tl_mark_pass(in->prefixes);
	_tl_mark_pass(in->env);
	_tl_mark_pass(in->top_env);
	_tl_mark_pass(in->conts);
	_tl_mark_pass(in->values);
	obj = in->top_alloc;
	while(obj) {
		tmp = obj;
		obj = tl_next_alloc(obj);
		if(!tl_is_marked(tmp)) {
			tl_free(in, tmp);
		}
	}
}

/** Returns the length of a list.
 *
 * This is the number of iterations that would be done by `tl_list_iter`.
 */
size_t tl_list_len(tl_object *l) {
	size_t cnt = 0;
	if(!l || !tl_is_pair(l)) return cnt;
	for(tl_list_iter(l, item)) cnt++;
	return cnt;
}

/** Reverses a list.
 *
 * This builds a list by pushing elements onto a running stack (another list)
 * in the order they are traversed, so it requires no extra space (beyond the
 * allocation of the returned list). It is, however, linear time.
 */
tl_object *tl_list_rvs(tl_interp *in, tl_object *l) {
	tl_object *res = TL_EMPTY_LIST;
	for(tl_list_iter(l, item)) res = tl_new_pair(in, item, res);
	return res;
}

/** Reverses a list, retaining the last item as its "improper" tail. */
tl_object *tl_list_rvs_improp(tl_interp *in, tl_object *l) {
	tl_object *res = tl_first(l);
	l = tl_next(l);
	for(tl_list_iter(l, item)) res = tl_new_pair(in, item, res);
	return res;
}

/** Internal function for computing the hash of data.
 *
 * This is used to accelerate symbol lookups.
 */
unsigned long tl_data_hash(const char *data, size_t len) {
	unsigned long ret = 0, a = 0x848c848c;
	unsigned char samt;
	size_t i, end = len > TL_HASH_LINEAR ? TL_HASH_LINEAR : len;

	for(i = 0; i < end; i++) {
		ret = (ret << 8) | (data[i] ^ (ret >> (sizeof(unsigned long) * 8 - 8)));
		if(i & 0x1) {
			ret ^= a;
			samt = ret & 0xF;
			a = (a << samt) | (a >> (sizeof(unsigned long) * 8 - samt));
		}
		ret = (ret << 8) | (data[len - i - 1] ^ (ret >> (sizeof(unsigned long) * 8 - 8)));
	}

	return ret;
}

/** Copies a C string into another C string.
 *
 * This is an analogue to `strdup` which uses the interpreter allocator
 * (tl_interp::mallocf).
 */
char *tl_strdup(tl_interp *in, const char *str) {
	size_t s;
	char *buf;

	if(!str) return NULL;

	s = strlen(str) + 1;
	if(!s) return NULL;

	buf = tl_alloc_malloc(in, s);
	if(!buf) {
		tl_gc(in);
		buf = tl_alloc_malloc(in, s);
		assert(buf);
	}

	return strcpy(buf, str);
}

/** Allocates an array of size `n` with elements of size `s`, initializing all bytes to 0.
 *
 * This is an analogue to `calloc` which uses the interpreter allocator
 * (tl_interp::mallocf).
 */
void *tl_calloc(tl_interp *in, size_t n, size_t s) {
	// FIXME: Don't multiply the two size_t's unchecked--if this overflows, bad things happen.
	void *region = tl_alloc_malloc(in, n * s);
	if(!region) return NULL;
	return memset(region, 0, n * s);
}
