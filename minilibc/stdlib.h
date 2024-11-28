#ifndef MINILIBC_STDLIB_H
#define MINILIBC_STDLIB_H

#include <stddef.h>

void *malloc(size_t);
void free(void *);
void *realloc(void *, size_t);
void exit(int);
void abort();

#ifdef MEM_INSPECT
struct meminfo {
	size_t size;
	size_t used;
	size_t allocated;
};

int meminfo(size_t, struct meminfo *);

#define HAS_MEMINFO
#endif

#endif
