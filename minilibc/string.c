#include "string.h"

#include <stdlib.h>

size_t strlen(const char *c) {
	size_t n = 0;
	while(*c++ != 0) n++;
	return n;
}

int strcmp(const char *a, const char *b) {
	while(*a && *b && *a++ == *b++) ;
	return *a - *b;
}

char *strcpy(char *dest, const char *src) {
	char *d = dest;
	while((*d++ = *src++)) ;
	return dest;
}

char *strpbrk(const char *s, const char *c) {
	const char *t;
	while(*s) {
		t = c;
		while(*t) {
			if(*t == *s) return (char *)s;
			t++;
		}
		s++;
	}
	return NULL;
}

void *memset(void *s, int c, size_t n) {
	unsigned char *d = s;
	while(n--) *d++ = (unsigned char) c;
	return s;
}

int memcmp(const void *c1, const void *c2, size_t n) {
	size_t i = 0;
	int diff;
	while(i < n) {
		if((diff = (((char *) c1)[i] - ((char *) c2)[i])))
			return diff;
		i++;
	}
	return 0;
}

void *memcpy(void *dest, const void *src, size_t n) {
	size_t i = 0;
	while(i < n) {
		((char *) dest)[i] = ((char *) src)[i];
		i++;
	}
	return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
	size_t i;
	if(src == dest) return dest;
	if(src < dest) {
		for(i = n - 1; i > 0; i--) {
			((char *)dest)[i] = ((char *)src)[i];
		}
		*((char *)dest) = *((char *)src);
	} else {
		memcpy(dest, src, n);
	}
	return dest;
}
