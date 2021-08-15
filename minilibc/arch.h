#ifndef MINILIBC_ARCH_H
#define MINILIBC_ARCH_H

#define EOF ((int) -1)

#define STDIN 1
#define STDOUT 2
#define STDERR 3

#define _HALT_ERROR 0xff

#define _NORETURN __attribute__((noreturn))

void arch_fflush(unsigned long);
void arch_fputc(unsigned long, char);
int arch_fgetc(unsigned long);
void _NORETURN arch_halt(int);
void arch_init_heap(void **, size_t *);

#endif
