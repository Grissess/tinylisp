#ifndef MINILIBC_STDLIB_H
#define MINILIBC_STDLIB_H

#include <stddef.h>

void *malloc(size_t);
void free(void *);
void *realloc(void *, size_t);

#endif
