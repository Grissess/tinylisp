#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "tinylisp.h"

static int _tl_refill_freelist(tl_interp *in, size_t objects) {
	/* XXX overflow hazard but we don't want memset here */
	tl_object *list = tl_alloc_malloc(in, objects * sizeof(tl_object));
	size_t index;
#ifdef GC_DEBUG
	tl_printf(in, "refill: 0x%zx objects at %p\n", objects, list);
#endif
	if(!list) return 0;
	for(index = 0; index < objects; index++) {
		list[index].next_alloc = in->free_alloc;
		list[index].prev_alloc = NULL;
		if(in->free_alloc) in->free_alloc->prev_alloc = list + index;
		in->free_alloc = list + index;
	}
	return 1;
}

/** Create a new object.
 *
 * The object has undefined type, which must be initialized. The more specific
 * constructors already do this.
 *
 * The returned object is threaded into the garbage collector, and will be
 * collected if it is not reachable from any root on the next call to `tl_gc`.
 */
tl_object *tl_new(tl_interp *in) {
	tl_object *obj = NULL;
	tl_trace(new_enter, in);

	/* Is the freelist empty? */
	if(!in->free_alloc) {
		if(!_tl_refill_freelist(in, in->oballoc_batch)) {
			/* If we're here, we can't allocate a full batch. Try just one
			 * object instead; we'll take it from the freelist momentarily. */
			if(!_tl_refill_freelist(in, 1)) {
				/* Still no luck? Try compacting our free memory; this is very expensive. */
				tl_gc(in);
				tl_reclaim(in);
				if(!_tl_refill_freelist(in, 1)) {
					/* We are well and truly out of memory; no sense in proceeding. */
					abort();
				}
			}
		}
	}

	/* Unthread from freelist */
	obj = in->free_alloc;
	in->free_alloc = obj->next_alloc;
	if(in->free_alloc) in->free_alloc->prev_alloc = NULL;

	/* Thread into alloc list */
	obj->next_alloc = in->top_alloc;
	obj->prev_alloc = NULL;
	if(in->top_alloc) in->top_alloc->prev_alloc = obj;
	in->top_alloc = obj;

	tl_trace(new_exit, in, obj);
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
	tl_buffer buf = {(char *) data, len};  /* Discard constness--it won't mutate anyway */
	return tl_new_sym_name(in, tl_ns_resolve(in, &in->ns, buf));
}

/** Creates a new symbol object by name.
 *
 * It's assumed that the name is unique to this symbol and was correctly
 * initialized, generally by resolving against the interpreter's own namespace.
 * This isn't enforced, however, and this mechanism could theoretically be used
 * to introduce entirely hygienic symbols.
 */
tl_object *tl_new_sym_name(tl_interp *in, tl_name *name) {
	tl_object *obj = tl_new(in);
	obj->kind = TL_SYM;
	obj->nm = name;
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

/** Creates a new pointer object.
 *
 * These objects are thin wrappers around a C pointer, generally opaque to the
 * interpreter and its language. They're primarily for extension developers to
 * embed application-specific data as values that can be manipulated as a TL
 * value: passed to functions, stored in lists, etc.
 *
 * The tag is stored in the tag field. You can allocate a tag for your own
 * usage (with ::tl_is_tag) using ::tl_new_tag, or you can use the reserved
 * ::TL_NO_TAG if you don't care about it. It has no semantic meaning beyond
 * this use.
 *
 * Note that TinyLISP can easily retain values indefinitely; garbage-collection
 * is only done when ::tl_gc is called, normally via tl_interp::gc_events .
 * Beside that, objects can be retained in surprising ways, such as the frames
 * captured by a continuation which is itself still reachable. Surrendering
 * this pointer to the runtime may well require that you ensure it stays alive
 * for potentially the lifetime of the entire interpreter. The only time it is
 * safe to completely free its referent is in the callback given, which is
 * invoked from ::tl_free . (Don't free the object itself in this callback,
 * only the pointer it contains.)
 *
 * Storing TinyLISP values inside of a pointer, even indirectly, is a bad idea;
 * there is no public-facing routine for allowing the GC mark pass to view
 * inside of this pointer, and so it may eventually contain dangling
 * references. If you want to expose multiple TL values, construct a list (see
 * ::tl_new_pair ). Lists can safely contain pointer objects, and are
 * compatible with GC.
 */
tl_object *tl_new_ptr(tl_interp *in, void *ptr, void (*gcfunc)(tl_interp *, tl_object *), tl_tag tag) {
	tl_object *obj = tl_new(in);
	obj->kind = TL_PTR;
	obj->ptr = ptr;
	obj->gcfunc = gcfunc;
	obj->tag = tag;
	return obj;
}

/** "Free" an object, adding it to a reclaimable freelist.
 *
 * TinyLISP has a tracing GC, so, as a rule, you should never need to do this.
 * However, you may do this if you can prove that you are removing the last
 * reference to this object, as a slight optimization. The amount of
 * optimization is rarely worth the proof (since TL code can easily capture
 * references you don't know about in many cases), so it's best to simply not
 * use this function (except from within `tl_gc`).
 *
 * Bizarre behavior can happen with references to objects in the freelist, so
 * we make an effort to poison them so their use causes issues early. In
 * particular, this list can be destroyed with ::tl_reclaim, so any such bug is
 * tantamount to a use-after-free.
 *
 * The underyling allocator's "free" is called from ::tl_reclaim. Under memory
 * pressure (as in ::tl_new), it is best to call ::tl_reclaim immediately after
 * ::tl_gc.
 */
void tl_free(tl_interp *in, tl_object *obj) {
	tl_trace(free_enter, in, obj);
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
	if(in->free_alloc) in->free_alloc->prev_alloc = obj;
	obj->next_alloc = in->free_alloc;
	obj->prev_alloc = NULL;
	in->free_alloc = obj;
	tl_trace(free_exit, in, obj);
}

/** Well and truly destroys an object, free()ing its memory.
 *
 * This is unsafe to do to any live object, and will manifest memory unsafety
 * and other undefined behavior if you do. It's best to only let ::tl_reclaim
 * call this function unless you know what you're doing.
 */
void tl_destroy(tl_interp *in, tl_object *obj) {
	tl_trace(destroy_enter, in, obj);
	switch(obj->kind) {
		case TL_CFUNC:
		case TL_CFUNC_BYVAL:
		case TL_THEN:
			tl_alloc_free(in, obj->name);
			break;

		case TL_PTR:
			if(obj->gcfunc) obj->gcfunc(in, obj);
			break;

		default:
			break;
	}
	tl_alloc_free(in, obj);
	tl_trace(destroy_exit, in, obj);
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
		case TL_PTR:
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
 * This calls ::tl_free on objects registered to the garbage collector (via
 * ::tl_new and related) but which are no longer reachable from the roots
 * (generally, all `tl_object *` in the interpreter).
 */
void tl_gc(tl_interp *in) {
	tl_object *obj = in->top_alloc;
	tl_object *tmp;
#ifdef GC_DEBUG
	size_t freed = 0;
	tl_printf(in, "gc: starts\n");
#endif
	tl_trace(gc_enter, in);
	while(obj) {
#if defined(GC_DEBUG) && GC_DEBUG > 0
		tl_printf(in, "gc: unmark: %p %O\n", obj, obj);
#endif
		tl_unmark(obj);
		obj = tl_next_alloc(obj);
	}
	tl_trace(gc_mark_enter, in);
	_tl_mark_pass(in->true_);
	_tl_mark_pass(in->false_);
	_tl_mark_pass(in->error);
	_tl_mark_pass(in->prefixes);
	_tl_mark_pass(in->env);
	_tl_mark_pass(in->top_env);
	_tl_mark_pass(in->current);
	_tl_mark_pass(in->conts);
	_tl_mark_pass(in->values);
	/* One could make a list of the permanent objects during the unmark scan
	 * above, but making said list would either (1) require allocation, which
	 * the GC should NOT do, or (2) require there to be a fixed-size array set
	 * aside somewhere for the purpose. Neither is a preferable option, so
	 * we'll just settle for scanning the object list one more time.
	 */
	obj = in->top_alloc;
	while(obj) {
		if(tl_is_permanent(obj)) _tl_mark_pass(obj);
		obj = tl_next_alloc(obj);
	}
	tl_trace(gc_mark_exit, in);
	obj = in->top_alloc;
	while(obj) {
		tmp = obj;
		obj = tl_next_alloc(obj);
		if(!tl_is_marked(tmp)) {
#ifdef GC_DEBUG
			freed++;
#if GC_DEBUG > 0
			tl_printf(in, "gc: free: %p %O\n", tmp, tmp);
#endif
#endif
			tl_free(in, tmp);
		}
	}
#ifdef GC_DEBUG
	tl_printf(in, "gc: freed 0x%zx objects\n", freed);
#endif
	tl_trace(gc_exit, in);
}

/** Reclaims memory from the interpreter.
 *
 * This is most effective after a GC, as it merely drops all entries in the
 * freelist that ::tl_gc has assembled from unreachable objects. Having these
 * objects cached is good for performance, but may be undesirable if the
 * program is under memory pressure; if malloc reports this, ::tl_new calls
 * this automatically. Thus, regular users shouldn't do this, as it hurts
 * optimization in the usual cases.
 */
void tl_reclaim(tl_interp *in) {
	tl_object *obj = in->free_alloc, *next;
	while(obj) {
		next = tl_next_alloc(obj);
		tl_destroy(in, obj);
		obj = next;
	}
}

/** Returns the length of a list.
 *
 * This is defined as the number of iterations that would be done by \ref
 * tl_list_iter . This might not be what you're expecting; for example, an
 * improper list has its tail counted toward its length,
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

/** Return a C string containing the symbol's name.
 *
 * This is allocated with the interpreter allocator, and needs to be freed with
 * ::tl_alloc_free . It's not tracked or GC'd otherwise; a fresh copy is
 * returned.
 *
 * On a failure, this returns NULL.
 */
char *tl_sym_to_cstr(tl_interp *in, tl_object *sym) {
	if(!tl_is_sym(sym)) return NULL;
	void *region = tl_alloc_malloc(in, sym->nm->here.len + 1);
	if(!region) return NULL;
	memcpy(region, sym->nm->here.data, sym->nm->here.len);
	((char *)region)[sym->nm->here.len] = 0;
	return region;
}
