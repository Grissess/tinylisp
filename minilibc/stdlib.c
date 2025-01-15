#include "stdlib.h"
#include "string.h"
#include "assert.h"
#include "stdio.h"
#include "arch.h"

#if defined(PTR_LSB_AVAILABLE) && PTR_LSB_AVAILABLE == 0
#define NO_MEM_PACK
#endif

#ifdef NO_MEM_PACK

#define fl_set_used(fl) ((fl)->is_used = 1)
#define fl_set_unused(fl) ((fl)->is_used = 0)
#define fl_next(fl) ((fl)->next)
#define fl_used(fl) ((fl)->is_used)
#define fl_make_next(fl, nptr) (nptr)

#else

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

#endif

#define BAD_SIZE ((size_t) -1)
#define fl_real_size(fl) ((size_t)(((fl) && fl_next((fl))) ? ((size_t) fl_next((fl))) - ((size_t) (fl)) : BAD_SIZE))
#define fl_size(fl) (fl_real_size((fl)) == BAD_SIZE || fl_real_size((fl)) < sizeof(struct freelist) ? BAD_SIZE : fl_real_size((fl)) - sizeof(struct freelist))

struct arena;

struct freelist {
	struct arena *arena;
	struct freelist *next;
	struct freelist *nextfree;
	struct freelist *prevfree;
#if defined(MEM_DEBUG) || defined(NO_MEM_PACK)
	char is_used;
#endif
#ifdef MEM_INSPECT
	size_t alloc;
#endif
};

struct arena {
	struct freelist *root, *freeroot;
	size_t size;
	struct arena *next;
#ifdef MEM_DEBUG
	struct freelist *sentinel;
#endif
#ifdef MEM_INSPECT
	size_t used;
	size_t allocated;
#endif
} *alist = NULL;

#ifdef MEM_DEBUG
#define tl_sentinel(arena) (arena)->sentinel
#else
#define tl_sentinel(arena) ((struct freelist *) 0)
#endif

#define MIN_HEADER_SIZE (sizeof(struct freelist) * 2 + sizeof(struct arena))

#ifdef MEM_DEBUG

static void mem_sanity(struct arena *arena) {
	struct freelist *cur, *prev;
	char *orig_root;
	size_t orig_size;
	struct freelist *end_ptr;

	if(!arena) {
		fprintf(stderr, "sanity: CRITICAL: NULL arena!\n");
		return;
	}

	orig_root = (char *) arena->root;
	orig_size = arena->size;
	end_ptr = arena->sentinel;

	if(((char *) arena) >= orig_root && ((char *) arena) < (orig_root + orig_size))
		fprintf(stderr, "sanity: arena %p is inside its own allocatable area %p to %p (size %p)\n", arena, orig_root, orig_root + orig_size, orig_size);
	if(arena->next && ((char *) arena->next) >= orig_root && ((char *) arena->next) < (orig_root + orig_size))
		fprintf(stderr, "sanity: arena %p has next arena %p inside its own allocatable area %p to %p (size %p)\n", arena, arena->next, orig_root, orig_root + orig_size, orig_size);
	if(arena->next && arena->next == arena)
		fprintf(stderr, "sanity: arena %p has 1-cycle next to %p\n", arena, arena->next);

	cur = arena->root;
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

	cur = arena->freeroot;
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

void *malloc_in_arena(struct arena *arena, size_t b) {
	struct freelist *cur, *next, *nf;
	void *area;
	char split = 1;
	size_t old;

	if(b == 0) return NULL;
	if(b & 0x7) b = (b + 7) & (~0x7);


	cur = arena->freeroot;
	while(cur && fl_size(cur) != BAD_SIZE && fl_size(cur) < b)
		cur = cur->nextfree;
	if(!cur || fl_size(cur) == BAD_SIZE) {
#ifdef MEM_DEBUG
		fprintf(stderr, "malloc: out of memory, cur = %p next = %p.\n", cur, cur ? fl_next(cur) : ((void *) -1));
#endif
		return NULL;
	}

	area = (void *)(cur + 1);
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
		arena->freeroot = nf;
		if(nf)
			nf->prevfree = NULL;
	}
	/* Force freelist errors to scream -- cur should NOT be in the list anymore */
	cur->nextfree = NULL;
	cur->prevfree = NULL;
	cur->arena = arena;
	fl_set_used(cur);

#ifdef MEM_DEBUG
	fprintf(stderr, "malloc: gave %p of %p bytes from arena %p root %p sz %p, next is %p (%s) (%p away) with %p, nextfree is %p with %p, fr is %p sz %p\n", area, b, arena, cur, old, next, split ? "split" : "not split", ((char *) next) - ((char *) cur), fl_size(next), nf, fl_size(nf), arena->freeroot, fl_size(arena->freeroot));
	mem_sanity(arena);
#endif
#ifdef MEM_INSPECT
	arena->used += split ? b + sizeof(struct freelist) : old;
	arena->allocated += b;
	cur->alloc = b;
#endif

	return area;
}

void *malloc(size_t b) {
	struct arena *arena = alist;
	void *area = NULL;

	for(; arena; arena = arena->next) {
		if(b > arena->size) continue;
		area = malloc_in_arena(arena, b);
		if(area) return area;
	}

	if(!arena) {  /* TODO: tautology */
		size_t s = 0;
		void *root = NULL;
		arch_new_heap(b + MIN_HEADER_SIZE, &root, &s);
#if defined(MEM_DEBUG) || defined(MEM_DEBUG_ARENAS)
		fprintf(stderr, "malloc: no extant arena could serve request of size %p; arch_new_heap returns root %p, size %p\n", b, root, s);
#endif
		if(!root) {
#if defined(MEM_DEBUG) || defined(MEM_DEBUG_ARENAS)
			fprintf(stderr, "malloc: out of heaps! failed request of size %p\n", b);
#endif
			return NULL;
		}
		if(s < MIN_HEADER_SIZE) {
#if defined(MEM_DEBUG) || defined(MEM_DEBUG_ARENAS)
			fprintf(stderr, "malloc: heap too small to alloc! failed request of size %p\n", b);
#endif
			arch_release_heap(root, s);
			return NULL;
		}
#if defined(MEM_DEBUG) || defined(MEM_DEBUG_ARENAS)
		fprintf(stderr, "malloc: init heap at %p size %p for request of size %p\n", root, s, b);
#endif
		arena = root;
		arena->size = s;
		arena->freeroot = (struct freelist *)(((char *) root) + s - sizeof(struct freelist));
		arena->freeroot->next = NULL;
		arena->freeroot->nextfree = NULL;
		arena->freeroot->prevfree = NULL;
		arena->root = (struct freelist *)(arena + 1);
		arena->root->next = arena->root->nextfree = arena->freeroot;
		arena->root->prevfree = NULL;
#ifdef MEM_DEBUG
		arena->freeroot->is_used = 0;
		arena->root->is_used = 0;
		arena->sentinel = arena->freeroot;
#endif
		arena->freeroot = arena->root;
		arena->next = alist;
		alist = arena;
#if defined(MEM_DEBUG) || defined(MEM_DEBUG_ARENAS)
		fprintf(stderr, "malloc: new arena %p root %p freeroot %p size %p next %p sentinel %p\n", arena, arena->root, arena->freeroot, arena->size, arena->next, tl_sentinel(arena));
#endif
	}
	area = malloc_in_arena(arena, b);
#if defined(MEM_DEBUG) || defined(MEM_DEBUG_ARENAS)
	if(!area) {
		fprintf(stderr, "malloc: newly allocated heap %p size %p cannot satisfy request for %p bytes, aborting\n", arena->root, arena->size, b);
	}
#endif
	return area;
}

void free(void *area) {
	struct freelist *fl = ((struct freelist *) area) - 1;
	size_t i = 1;

	if(!area) return;

#ifdef MEM_DEBUG
	fprintf(stderr, "free: going to release %p at %p\n", area, fl); // Get this out first
	fprintf(stderr, "free: ... size %p, %s, arena %p, next %p, nf %p, pf %p\n", fl_size(fl), fl_used(fl) ? "used" : "NOT USED", fl->arena, fl_next(fl), fl->nextfree, fl->prevfree);
#endif

#ifdef MEM_INSPECT
	fl->arena->used -= fl_size(fl);
	fl->arena->allocated -= fl->alloc;
#endif

	fl_set_unused(fl);
	while(fl_next(fl) && fl_next(fl_next(fl)) && !fl_used(fl_next(fl))) {
		/* Unlink from freelist */
		struct freelist *pf = fl_next(fl)->prevfree, *nf = fl_next(fl)->nextfree;
		if(!(pf || nf))
			fprintf(stderr, "free: WARN: found entry %p sz %p from %p sz %p that has double NULL freelist pointers\n", fl_next(fl), (void*)fl_size(fl_next(fl)), fl, (void*)fl_size(fl));
		if(pf)
			pf->nextfree = nf;
		else
			fl->arena->freeroot = nf;
		if(nf)
			nf->prevfree = pf;
		/* And coalesce the area */
		fl->next = fl_make_next(fl, fl_next(fl_next(fl)));
		i++;
#ifdef MEM_INSPECT
		fl->arena->used -= sizeof(struct freelist);
#endif
	}

	fl->nextfree = fl->arena->freeroot;
	fl->prevfree = NULL;
	fl->arena->freeroot->prevfree = fl;
	fl->arena->freeroot = fl;

#ifdef MEM_DEBUG
	fprintf(stderr, "free: freed %p releasing %p bytes in %p blocks to arena %p, fr now %p sz %p, nextfree %p sz %p\n", area, fl_size(fl), i, fl->arena, fl->arena->freeroot, fl_size(fl->arena->freeroot), fl->arena->freeroot->nextfree, fl_size(fl->arena->freeroot->nextfree));
	mem_sanity(fl->arena);
#endif
}

void *realloc(void *ptr, size_t n) {
	void *new;
	size_t sz;

	if(!n) {
		free(ptr);
		return NULL;
	}
	if(!ptr) {
		return malloc(n);
	}

	/* TODO: be more clever about expanding/contracting the allocation */
	sz = fl_size(((struct freelist *)ptr) - 1);
	if(n <= sz) {
		/* Could shrink the allocation here */
		return ptr;
	}
	new = malloc(n);
	memcpy(new, ptr, sz);
	return new;
}

#ifdef MEM_INSPECT

int meminfo(size_t index, struct meminfo *minfo) {
	struct arena *arena = alist;

	if(!minfo) return 0;

	while(arena && index) {
		arena = arena->next;
		index--;
	}
	if(!arena) return 0;

	minfo->size = arena->size;
	minfo->used = arena->used;
	minfo->allocated = arena->allocated;

	return 1;
}

#endif

void exit(int status) {
	fprintf(stderr, "exit: program called exit() with status %d\n", status);
	arch_halt(status);
}

void abort() {
	fprintf(stderr, "abort: program aborted\n");
	arch_halt(_HALT_ERROR);
}
