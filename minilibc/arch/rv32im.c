#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../arch.h"

#ifndef JTAG_BASE
#define JTAG_BASE 0x100000
#endif

#ifndef MEM_BASE
#define MEM_BASE 0x300000
#endif

#ifndef MEM_SIZE
#define MEM_SIZE 0x100000
#endif

#ifndef STACK_WORDS
#define STACK_WORDS 0x1000
#endif

#define JTAG ((volatile uint32_t *const) JTAG_BASE)
#define JTF_READY (1<<15)
#define JTF_CHAR (0x7f)

void arch_fflush(unsigned long _) {}

void arch_fputc(unsigned long hdl, char c) {
	if(hdl == STDOUT || hdl == STDERR) {
		*JTAG = JTF_CHAR & c;
	}
}

int arch_fgetc(unsigned long hdl) {
	uint32_t value;
	if(hdl != STDIN) return EOF;
	while(1) {
		value = *JTAG;
		if(value & JTF_READY) return JTF_CHAR & value;
	}
}

void arch_halt(int _) { while(1); }

static int arch_heap_was_init = 0;

void arch_new_heap(size_t min_sz, void **region, size_t *sz) {
	if(!arch_heap_was_init) {
		arch_heap_was_init = 1;
		*region = (void *)MEM_BASE;
		*sz = MEM_SIZE;
	} else {
		fprintf(stderr, "%s: no more heap\n", __func__);
	}
}

void arch_release_heap(void *region, size_t sz) {
	arch_heap_was_init = 0;
}

uint32_t stack[STACK_WORDS];
uint32_t *stack_top = stack + STACK_WORDS;

int main();
__asm__(
		".global _start\n"
		".section begin\n"
		"_start:\n"
		"    la sp, stack_top\n"
		"    mv fp, zero\n"
		"    call main\n"
		"    li a0, 0\n"
		"    call arch_halt\n"
);
