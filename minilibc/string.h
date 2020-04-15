#ifndef MINILIBC_STRING_H
#define MINILIBC_STRING_H

#include <stddef.h>

size_t strlen(const char *);
int strcmp(const char *, const char *);
char *strcpy(char *, const char *);
char *strpbrk(const char *, const char *);

void *memset(void *, int, size_t);
int memcmp(const void *, const void *, size_t);
void *memcpy(void *, const void *, size_t);

#endif
