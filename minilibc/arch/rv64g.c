#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../arch.h"

#ifndef UART_BASE
#define UART_BASE 0x10000000
#endif

#ifndef MEM_SIZE
#define MEM_SIZE 256*1024*1024
#endif

#define UART ((volatile uint8_t *) UART_BASE)
#define UART_XR 0
#define UART_LSR 5
#define ULSR_THRE 0x20
#define ULSR_DRDY 0x01

void arch_fflush(unsigned long _) {}

void arch_fputc(unsigned long hdl, char c) {
	if(hdl == STDOUT || hdl == STDERR) {
		while(!(UART[UART_LSR] & ULSR_THRE));
		UART[UART_XR] = c;
	}
}

int arch_fgetc(unsigned long hdl) {
	if(hdl != STDIN) return EOF;
	while(!(UART[UART_LSR] & ULSR_DRDY));
	return UART[UART_XR];
}

void arch_halt(int _) { while(1); }

extern uint8_t heap_start;
static int heap_init = 0;

void arch_new_heap(size_t min, void **region, size_t *sz) {
	if(!heap_init) {
		heap_init = 1;
		*region = &heap_start;
		*sz = MEM_SIZE;
	} else {
		fprintf(stderr, "%s: no more heap\n", __func__);
	}
}

void arch_release_heap(void *region, size_t sz) {
	heap_init = 0;
}

int main();
__asm__(
		".global _start\n"
		".section begin\n"
		"_start:\n"
		"	call main\n"
		"	li a0, 0\n"
		"	call arch_halt\n"
);
