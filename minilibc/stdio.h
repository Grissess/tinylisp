#ifndef MINILIB_STDIO_H
#define MINILIB_STDIO_H

#include <stddef.h>

typedef void FILE;

extern FILE *stdin, *stdout, *stderr;

void fflush(FILE *);
int fputc(int, FILE *);
#define putc(i, f) fputc((i), (f))
#define putchar(i) fputc((i), stdout)
int fgetc(FILE *);
#define getc(f) fgetc((f))
#define getchar() fgetc(stdin)
int fputs(const char *, FILE *);
int puts(const char *);
size_t fwrite(const void *, size_t, size_t, FILE *);

int snprintf(char *, size_t, const char *, ...);
int fprintf(FILE *, const char *, ...);
#define printf(...) fprintf(stdout, __VA_ARGS__)

#endif
