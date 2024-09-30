#include "stdio.h"
#include "string.h"
#include "arch.h"
#include "errno.h"

#include <stdarg.h>

FILE *stdin = (FILE *) 1, *stdout = (FILE *) 2, *stderr = (FILE *) 3;

void fflush(FILE *f) {
	arch_fflush((unsigned long) f);
}

int fputc(int c, FILE *f) {
	arch_fputc((unsigned long) f, c);
	return c;
}

int fgetc(FILE *f) {
	return arch_fgetc((unsigned long) f);
}

int fputs(const char *s, FILE *f) {
	while(*s) arch_fputc((unsigned long) f, *s++);
	return 0;
}

int puts(const char *s) {
	fputs(s, stdout);
	putchar('\n');
	return 0;
}

size_t fwrite(const void *p, size_t s, size_t n, FILE *f) {
	size_t c, r = n;
	const unsigned char *d = p;
	while(n--) for(c = 0; c < s; c++) arch_fputc((unsigned long) f, *d++);
	return r;
}

size_t fread(void *p, size_t s, size_t n, FILE *f) {
	size_t c, r = n;
	unsigned char *d = p;
	while(n--) for(c = 0; c < s; c++) *d++ = arch_fgetc((unsigned long) f);
	return r;
}

FILE *fopen(const char *name, const char *mode) {
	if(!strcmp(name, "/dev/stdin") && *mode == 'r') return stdin;
	if(!strcmp(name, "/dev/stdout") && *mode == 'w') return stdout;
	if(!strcmp(name, "/dev/stderr") && *mode == 'w') return stderr;
	errno = ENOENT;
	return NULL;
}

int fclose(FILE *fp) { return 0; }

#define VXP_NAME vsnprintf
#define VXP_ARGS char *s, size_t sz,
/* Don't return early on fill; we're depending on an accurate cnt
 * Also beware the careful unsigned comparison when sz == 0
 */
#define VXP_PUTC(c) do {\
	if(cnt + 1 >= sz) {\
		if(sz && s) s[cnt] = 0;\
	} else if(s) s[cnt] = (c);\
} while(0)
#define VXP_DONE if(s && sz && cnt < sz) s[cnt] = 0
#include "vxprintf_impl.c"

#define VXP_NAME vfprintf
#define VXP_ARGS FILE *f,
#define VXP_PUTC(c) arch_fputc((unsigned long) f, (c))
#define VXP_DONE
#include "vxprintf_impl.c"

int snprintf(char *s, size_t sz, const char *fmt, ...) {
	va_list ap;
	int res;
	va_start(ap, fmt);
	res = vsnprintf(s, sz, fmt, ap);
	va_end(ap);
	return res;
}

int fprintf(FILE *f, const char *fmt, ...) {
	va_list ap;
	int res;
	va_start(ap, fmt);
	res = vfprintf(f, fmt, ap);
	va_end(ap);
	return res;
}
