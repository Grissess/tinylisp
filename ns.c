#include <string.h>
#include <assert.h>

#include "tinylisp.h"

tl_buffer tl_buf_slice(tl_interp *in, tl_buffer orig, size_t start, size_t end) {
	tl_buffer ret;

#ifdef NS_DEBUG
	tl_printf(in, "tl_buf_slice: %N start %ld end %ld\n", &orig, start, end);
#endif
	assert(start < orig.len && end <= orig.len && start < end);
	ret.len = end - start;
	ret.data = tl_alloc_malloc(in, ret.len);
	assert(ret.data);
	memcpy(ret.data, orig.data + start, ret.len);
	return ret;
}

static tl_name *tl_ns_split(tl_interp *in, tl_child *child, size_t len) {
	tl_name *new_name;

#ifdef NS_DEBUG
	tl_printf(in, "tl_ns_split: child here %N seg %N at len %ld\n", &child->name->here, &child->seg, len);
#endif
	assert(len > 0 && len < child->seg.len);

	/* Create the new parent (the current name will be retained for pointer stability) */
	new_name = tl_alloc_malloc(in, sizeof(tl_name));
	assert(new_name);
	/* Set its only child as the current node */
	new_name->num_children = new_name->sz_children = 1;
	new_name->children = tl_alloc_malloc(in, sizeof(tl_child));
	assert(new_name->children);
	new_name->children->name = child->name;
	/* Name the new node */
	new_name->here = tl_buf_slice(in, child->name->here, 0, child->name->here.len - child->seg.len + len);
	/* Copy the suffix into the new child */
	new_name->children->seg = tl_buf_slice(in, child->seg, len, child->seg.len);
	/* ...and reallocate the current segment */
	child->seg.len = len;
	child->seg.data = tl_alloc_realloc(in, child->seg.data, len);
	/* Thread in our split node as this segment's child */
	child->name = new_name;

	return new_name;
}

tl_name *tl_ns_resolve(tl_interp *in, tl_ns *ns, tl_buffer name) {
	tl_name *cur = ns->root;
	tl_child *children;
	size_t low, high, index;
	int sign;
	tl_buffer whole_name = name;

recurse:
#ifdef NS_DEBUG
	tl_printf(in, "tl_ns_resolve: name %N cur here %N\n", &name, &cur->here);
#endif
	if(!name.len) {
#ifdef NS_DEBUG
		tl_printf(in, "tl_ns_resolve: Found node: %N == %p.\n", &whole_name, cur);
#endif
		return cur;  /* Empty name means we've arrived. */
	}

	children = cur->children;
	low = 0;
	high = cur->num_children;
	while(1) {
		index = (low + high) / 2;
#ifdef NS_DEBUG
		tl_printf(in, " ... low %ld, index %ld, high %ld\n", low, index, high);
#endif
		if(index >= cur->num_children) break;
		sign = memcmp(name.data, children[index].seg.data, tl_min(name.len, children[index].seg.len));
#ifdef NS_DEBUG
		tl_printf(in, " ... sign %d\n", sign);
#endif
		if(!sign) {
			size_t match_len = tl_min(name.len, children[index].seg.len);
#ifdef NS_DEBUG
			tl_printf(in, " ... match_len %ld\n", match_len);
#endif
			if(name.len < children[index].seg.len) {  /* Exact prefix -- need to split */
				cur = tl_ns_split(in, children + index, name.len);
			} else {  /* March down the trie */
				cur = children[index].name;
			}
			name.data += match_len;
			name.len -= match_len;
			goto recurse;
		}
		if(sign > 0) {
#ifdef NS_DEBUG
			tl_printf(in, "... bsearch toward high\n");
#endif
			if(low == index) {
				if(low == high) {
					break;
				} else {
					/* We're off by one because of floor division, so
					 * increment. */
					low++;
				}
			} else {
				low = index;
			}
		} else {
#ifdef NS_DEBUG
			tl_printf(in, "... bsearch toward low\n");
#endif
			if(high == index) {
				break;
			} else {
				high = index;
			}
		}
	}
	/* If we fell off the loop without finding even a partial match, low ==
	 * high (and this is where this new segment needs to be inserted).
	 * There's a good chance that the element at the insertion index (if it
	 * exists) could contain a prefix and need to be split. */
#ifdef NS_DEBUG
	tl_printf(in, "tl_ns_resolve: insert at %ld into %p with %ld children (%ld cap)\n", low, cur, cur->num_children, cur->sz_children);
#endif
	if(low < cur->num_children) {
		size_t matching = 0;
		while(matching < name.len && matching < cur->children[low].seg.len && name.data[matching] == cur->children[low].seg.data[matching]) {
			matching++;
		}
#ifdef NS_DEBUG
		tl_printf(in, " ... matching %ld\n", matching);
#endif
		if(matching > 0) {
			/* Needs to be split. Note that this will end up changing the
			 * insertion index, so we tail recurse again. This will hopefully
			 * just fall out of the loop again, but we're guaranteed to not
			 * need another split if this went well. */
			if(matching == cur->children[low].seg.len) {
				/* There's a chance we can match the whole thing if this is the
				 * only child. In that case, no split; just point to the next
				 * node. */
				cur = cur->children[low].name;
			} else {
				cur = tl_ns_split(in, cur->children + low, matching);
			}
			name.data += matching;
			name.len -= matching;
#ifdef NS_DEBUG
			tl_printf(in, " ... cur split to %p\n", cur);
#endif
			goto recurse;
		}
	}
	/* There's also a chance that the matching segment is lesser, instead
	 * of greater; it will still be adjacent. Check that now. */
	if(low > 0) {
		size_t matching = 0;  /* Redundant */
		while(matching < name.len && matching < cur->children[low - 1].seg.len && name.data[matching] == cur->children[low - 1].seg.data[matching]) {
			matching++;
		}
		if(matching > 0) {
			if(matching == cur->children[low - 1].seg.len) {
				cur = cur->children[low - 1].name;
			} else {
				cur = tl_ns_split(in, cur->children + (low - 1), matching);
			}
			name.data += matching;
			name.len -= matching;
#ifdef NS_DEBUG
			tl_printf(in, " ... cur split (previous index) to %p\n", cur);
#endif
			goto recurse;
		}
	}
	if(cur->num_children >= cur->sz_children) {  /* Have to grow our vector */
#ifdef NS_DEBUG
		tl_child *old = cur->children;
#endif
		cur->sz_children = (cur->sz_children << 1) | 1;
		cur->children = tl_alloc_realloc(in, cur->children, sizeof(tl_child) * cur->sz_children);
		assert(cur->children);
#ifdef NS_DEBUG
		tl_printf(in, " ... sz_children grown to %ld (old %p, new %p)\n", cur->sz_children, old, cur->children);
#endif
	}
#ifdef NS_DEBUG
	{
		size_t i;
		tl_printf(in, " ... (children previously here: ");
		for(i = 0; i < cur->num_children; i++) {
			tl_printf(in, "%N(%p) ", &cur->children[i].seg, cur->children[i].name);
		}
		tl_printf(in, ")\n");
	}
#endif
	/* XXX amortize this */
	if(low < cur->num_children) {  /* Inserting, not appending */
#ifdef NS_DEBUG
		tl_printf(in, " ... move %ld units of size %ld to %ld from %ld\n", cur->num_children - low, sizeof(tl_child), low + 1, low);
#endif
		memmove(cur->children + low + 1, cur->children + low, sizeof(tl_child) * (cur->num_children - low));
#ifdef NS_DEBUG
		{
			size_t i;
			tl_printf(in, " ... (children after move: ");
			for(i = 0; i < cur->num_children + 1; i++) {
				tl_printf(in, "%N(%p) ", &cur->children[i].seg, cur->children[i].name);
			}
			tl_printf(in, ")\n");
		}
#endif
	}
#ifdef NS_DEBUG
	tl_printf(in, " ... alloc child of seg %N at index %ld (of %ld, cap %ld)\n", &name, low, cur->num_children, cur->sz_children);
#endif
	assert(low <= cur->num_children && low < cur->sz_children);
	cur->num_children++;
	cur->children[low].seg = tl_buf_slice(in, name, 0, name.len);
	cur->children[low].name = tl_alloc_malloc(in, sizeof(tl_name));
	assert(cur->children[low].name);
#ifdef NS_DEBUG
	{
		size_t i;
		tl_printf(in, " ... (children now here: ");
		for(i = 0; i < cur->num_children; i++) {
			tl_printf(in, "%N(%p) ", &cur->children[i].seg, cur->children[i].name);
		}
		tl_printf(in, ")\n");
	}
#endif
	cur = cur->children[low].name;
	cur->here = tl_buf_slice(in, whole_name, 0, whole_name.len);
	cur->num_children = cur->sz_children = 0;
	cur->children = NULL;

#ifdef NS_DEBUG
	tl_printf(in, "tl_ns_resolve: Created node: %N == %p.\n", &whole_name, cur);
#endif
	return cur;
}

void tl_ns_init(tl_interp *in, tl_ns *ns) {
	ns->root = tl_alloc_malloc(in, sizeof(tl_name));
	ns->root->here.data = NULL;
	ns->root->here.len = 0;
	ns->root->num_children = ns->root->sz_children = 0;
	ns->root->children = NULL;
}

void tl_ns_free(tl_interp *in, tl_ns *ns) {
	tl_name *cur, *temp;
	tl_child *child;
	size_t index;

	if(!(ns && ns->root)) return;
	cur = ns->root;
	cur->chain = NULL;

	while(cur) {
		for(index = 0; index < cur->num_children; index++) {
			child = &cur->children[index];
			tl_alloc_free(in, child->seg.data);
			child->name->chain = cur->chain;
			cur->chain = child->name;
		}
		tl_alloc_free(in, cur->children);
		tl_alloc_free(in, cur->here.data);
		temp = cur;
		cur = cur->chain;
		tl_alloc_free(in, temp);
	}
}

static void tl_ns_print_child(tl_interp *, tl_child *, size_t);

static void tl_ns_print_name(tl_interp *in, tl_name *nm, size_t lev) {
	size_t i;

	for(i = 0; i < lev; i++) {
		tl_puts(in, "  ");
	}
	if(!nm) {
		tl_printf(in, "[NULL]\n");
		return;
	}
	tl_printf(in, "%N\n", &nm->here);

	for(i = 0; i < nm->num_children; i++) {
		tl_ns_print_child(in, nm->children + i, lev);
	}
}

static void tl_ns_print_child(tl_interp *in, tl_child *child, size_t lev) {
	size_t i;

	for(i = 0; i < lev; i++) {
		tl_puts(in, "  ");
	}

	if(!child) {
		tl_printf(in, " <NULL>\n");
		return;
	}

	tl_printf(in, " -- %N -->\n", &child->seg);
	tl_ns_print_name(in, child->name, lev + 1);
}

void tl_ns_print(tl_interp *in, tl_ns *ns) {
	tl_ns_print_name(in, ns->root, 0);
}

void tl_ns_for_each(tl_interp *in, tl_ns *ns, void (*cb)(tl_interp *, tl_ns *, tl_name *, void *), void *data) {
	size_t i;
	tl_name *cur = ns->root;
	cur->chain = NULL;

	while(cur) {
		for(i = 0; i < cur->num_children; i++) {
			cur->children[i].name->chain = cur->chain;
			cur->chain = cur->children[i].name;
		}
		cb(in, ns, cur, data);
		cur = cur->chain;
	}
}

#ifdef DEBUG
static void _tl_add_symbol(tl_interp *in, tl_ns *_ns, tl_name *name, void *data) {
	tl_object *cell = (tl_object *)data;
	cell->first = tl_new_pair(in, tl_new_sym_name(in, name), cell->first);
}

TL_CF(all_symbols, "all-symbols") {
	tl_object *cell = tl_new_pair(in, TL_EMPTY_LIST, TL_EMPTY_LIST);
	tl_ns_for_each(in, &in->ns, _tl_add_symbol, cell);
	tl_cfunc_return(in, tl_first(cell));
}

TL_CF(print_ns, "print-ns") {
	tl_ns_print(in, &in->ns);
	tl_cfunc_return(in, in->true_);
}
#endif

#ifdef NS_TEST
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BUF_CAP 4096
int main() {
	tl_interp in;
	tl_ns ns;
	tl_buffer buf;
	tl_name *name;
	char *nl;

	buf.data = malloc(BUF_CAP);
	assert(buf.data);

	tl_interp_init(&in);
	tl_ns_init(&in, &ns);
	tl_ns_print(&in, &ns);

	while(fgets(buf.data, BUF_CAP, stdin)) {
		if((nl = strchr(buf.data, '\n'))) *nl = 0;
		buf.len = strlen(buf.data);
		name = tl_ns_resolve(&in, &ns, buf);
		tl_printf(&in, "%N -> %p\n", &buf, name);
		tl_ns_print(&in, &ns);
	}
	
	tl_ns_free(&in, &ns);
	tl_interp_cleanup(&in);

	return 0;
}
#endif
