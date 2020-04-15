#include "stdlib.h"
#include "string.h"
#include "assert.h"
#include "stdio.h"
#include "arch.h"

#define MF_INUSE 1
#define MF_ALL (MF_INUSE)
#define fl_next(fl) ((struct freelist *)(((size_t)(fl)->next)&~MF_ALL))
#define fl_used(fl) (((size_t)(fl)->next)&MF_INUSE)
#define fl_make_next(fl, nptr) (struct freelist *)((((size_t)(fl)->next)&MF_INUSE)|(((size_t)nptr)&~MF_ALL))

#ifdef MEM_DEBUG
#define fl_set_used(fl) ((fl)->next = (struct freelist *)(((size_t)(fl)->next)|MF_INUSE), (fl)->is_used = 1)
#define fl_set_unused(fl) ((fl)->next = (struct freelist *)(((size_t)(fl)->next)&~MF_INUSE), (fl)->is_used = 0)
#else
#define fl_set_used(fl) ((fl)->next = (struct freelist *)(((size_t)(fl)->next)|MF_INUSE))
#define fl_set_unused(fl) ((fl)->next = (struct freelist *)(((size_t)(fl)->next)&~MF_INUSE))
#endif

#define BAD_SIZE ((size_t) -1)
#define fl_real_size(fl) ((size_t)(((fl) && fl_next((fl))) ? ((size_t) fl_next((fl))) - ((size_t) (fl)) : BAD_SIZE))
#define fl_size(fl) (fl_real_size((fl)) == BAD_SIZE || fl_real_size((fl)) < sizeof(struct freelist) ? BAD_SIZE : fl_real_size((fl)) - sizeof(struct freelist))

struct freelist {
	struct freelist *next;
	struct freelist *nextfree;
	struct freelist *prevfree;
#ifdef MEM_DEBUG
	char is_used;
#endif
} *root = NULL, *freeroot = NULL;

#ifdef MEM_DEBUG
char *orig_root = NULL;
size_t orig_size = 0;
struct freelist *end_ptr;

static void mem_sanity() {
	struct freelist *cur, *prev;

	cur = root;
	while(cur) {
		if(((char *) cur) < orig_root || ((char *) cur) >= (orig_root + orig_size)) {
			fprintf(stderr, "sanity: CRITICAL: %p outside of original arena %p - %p -- while scanning root list -- aborting test\n", cur, orig_root, orig_root + orig_size);
			break;
		}
		if(fl_next(cur) && ((size_t) fl_next(cur)) < ((size_t) cur))
			fprintf(stderr, "sanity: %p has backward-pointing next %p\n", cur, fl_next(cur));
		if((!!fl_used(cur)) ^ (!!cur->is_used))
			fprintf(stderr, "sanity: %p disagrees between used bit (%p) and used flag (%p)\n", cur, fl_used(cur), (size_t)(cur->is_used));
		if(fl_next(cur) && fl_size(cur) >= orig_size)
			fprintf(stderr, "sanity: %p has size %p much larger than original arena size %p\n", cur, fl_size(cur), orig_size);
		if(!fl_next(cur) && cur != end_ptr)
			fprintf(stderr, "sanity: %p has no next but is not the final marker %p\n", cur, end_ptr);
		cur = fl_next(cur);
	}

	cur = freeroot;
	prev = NULL;
	while(cur) {
		if(((char *) cur) < orig_root || ((char *) cur) >= (orig_root + orig_size)) {
			fprintf(stderr, "sanity: CRITICAL: %p outside of original arena %p - %p -- while scanning freelist -- aborting test\n", cur, orig_root, orig_root + orig_size);
			break;
		}
		if(fl_used(cur) || cur->is_used)
			fprintf(stderr, "sanity: %p is used but on freelist\n", cur);
		if(prev != cur->prevfree)
			fprintf(stderr, "sanity: %p has bad freelist prev of %p when visited from %p\n", cur, cur->prevfree, prev);
		if(!(cur->prevfree || cur->nextfree))
			fprintf(stderr, "sanity: %p was visited but is completely unlinked in freelist\n", cur);
		if(!cur->nextfree && cur != end_ptr)
			fprintf(stderr, "sanity: %p has no freelist next but is not the final marker %p\n", cur, end_ptr);
		prev = cur;
		cur = cur->nextfree;
	}
}
#endif

void *malloc(size_t b) {
	struct freelist *cur, *next, *nf;
	void *area;
	char split = 1;
	size_t old;

	if(b == 0) return NULL;
	if(b & 0x7) b = (b + 7) & (~0x7);

	if(!root) {
		size_t s;
		arch_init_heap((void **) &root, &s);
		assert(root);
		freeroot = (struct freelist *)(((char *) root) + s - sizeof(struct freelist));
		freeroot->next = NULL;
		freeroot->nextfree = NULL;
		freeroot->prevfree = root;
		root->next = freeroot;
		root->nextfree = freeroot;
		root->prevfree = NULL;
#ifdef MEM_DEBUG
		orig_root = (void *) root;
		orig_size = s;
		freeroot->is_used = 0;
		root->is_used = 0;
		end_ptr = freeroot;
#endif
		freeroot = root;
	}

	cur = freeroot;
	while(cur && fl_size(cur) != BAD_SIZE && fl_size(cur) < b)
		cur = cur->nextfree;
	if(!cur || fl_size(cur) == BAD_SIZE) {
#ifdef MEM_DEBUG
		fprintf(stderr, "malloc: out of memory, cur = %p next = %p.\n", cur, cur ? fl_next(cur) : -1);
#endif
		return NULL;
	}

	area = ((char *) cur) + sizeof(struct freelist);
	old = fl_size(cur);
	if(b + sizeof(struct freelist) < fl_size(cur)) {
		next = (struct freelist *)(((char *) cur) + sizeof(struct freelist) + b);
		nf = next;
		next->next = fl_make_next(cur, fl_next(cur));
		next->nextfree = cur->nextfree;
		if(cur->nextfree)
			cur->nextfree->prevfree = next;
		fl_set_unused(next);
		cur->next = fl_make_next(cur, next);
	} else {
		next = fl_next(cur);
		nf = cur->nextfree;
		split = 0;
	}
	if(cur->prevfree) {
		cur->prevfree->nextfree = nf;
		if(nf)
			nf->prevfree = cur->prevfree;
	} else {
		freeroot = nf;
		if(nf)
			nf->prevfree = NULL;
	}
	/* Force freelist errors to scream -- cur should NOT be in the list anymore */
	cur->nextfree = NULL;
	cur->prevfree = NULL;
	fl_set_used(cur);

#ifdef MEM_DEBUG
	fprintf(stderr, "malloc: gave %p of %p bytes from %p sz %p, next is %p (%s) (%p away) with %p, nextfree is %p with %p, fr is %p sz %p\n", area, b, cur, old, next, split ? "split" : "not split", ((char *) next) - ((char *) cur), fl_size(next), nf, fl_size(nf), freeroot, fl_size(freeroot));
	mem_sanity();
#endif

	return area;
}

void free(void *area) {
	struct freelist *fl = (struct freelist *)(((char *) area) - sizeof(struct freelist));
	size_t i = 1;

	if(!area) return;

	fl_set_unused(fl);
	while(fl_next(fl) && fl_next(fl_next(fl)) && !fl_used(fl_next(fl))) {
		/* Unlink from freelist */
		struct freelist *pf = fl_next(fl)->prevfree, *nf = fl_next(fl)->nextfree;
		if(!(pf || nf))
			fprintf(stderr, "free: WARN: found entry %p sz %p from %p sz %p that has double NULL freelist pointers\n", fl_next(fl), fl_size(fl_next(fl)), fl, fl_size(fl));
		if(pf)
			pf->nextfree = nf;
		else
			freeroot = nf;
		if(nf)
			nf->prevfree = pf;
		/* And coalesce the area */
		fl->next = fl_make_next(fl, fl_next(fl_next(fl)));
		i++;
	}

	fl->nextfree = freeroot;
	fl->prevfree = NULL;
	freeroot->prevfree = fl;
	freeroot = fl;

#ifdef MEM_DEBUG
	fprintf(stderr, "free: freed %p releasing %p bytes in %p blocks, fr now %p sz %p, nextfree %p sz %p\n", area, fl_size(fl), i, freeroot, fl_size(freeroot), freeroot->nextfree, fl_size(freeroot->nextfree));
	mem_sanity();
#endif
}
