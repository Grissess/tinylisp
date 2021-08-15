#include <stddef.h>
#include <stdint.h>

#include "../arch.h"

#define SYS_READ 0
#define SYS_WRITE 1
#define SYS_EXIT 60
#define SYS_MMAP 9

#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define PROT_EXEC 0x4

#define MAP_SHARED 0x1
#define MAP_PRIVATE 0x2
#define MAP_ANONYMOUS 0x20

#define MAP_FAILED ((void *) -1)

#ifndef HEAP_SIZE
#define HEAP_SIZE (256*1024*1024)
#endif

void arch_fflush(unsigned long _) {}

static int fd_of(unsigned long hdl) {
	switch(hdl) {
	case STDIN:
		return 0;
	case STDOUT:
		return 1;
	case STDERR:
		return 2;
	default:
		return -1;
	}
}

void arch_fputc(unsigned long hdl, char c) {
	int fd = fd_of(hdl);
	if(fd < 0) return;

	register void *sysno asm ("rax") = (void *) SYS_WRITE;
	register void *fdesc asm ("rdi") = (void *) (size_t) fd;  // Double cast squelches a warning
	register void *buf asm ("rsi") = (void *) &c;
	register void *sz asm ("rdx") = (void *) 1;
	register void *ret asm ("rax");
	asm volatile ("syscall" : "=r" (ret) : "r" (sysno), "r" (fdesc), "r" (buf), "r" (sz));
	if(((long) ret) < 0) arch_halt(_HALT_ERROR);
}

int arch_fgetc(unsigned long hdl) {
	int fd = fd_of(hdl);
	char c;
	if(fd < 0) return EOF;

	register void *sysno asm ("rax") = (void *) SYS_READ;
	register void *fdesc asm ("rdi") = (void *) (size_t) fd;
	register void *buf asm ("rsi") = (void *) &c;
	register void *sz asm ("rdx") = (void *) 1;
	register void *ret asm ("rax");
	asm volatile ("syscall" : "=r" (ret) : "r" (sysno), "r" (fdesc), "r" (buf), "r" (sz));
	if(((long) ret) <= 0) return EOF;
	return c;
}

void arch_halt(int status) {
	while(1) {
		register void *sysno asm ("rax") = (void *) SYS_EXIT;
		register void *stat asm ("rdi") = (void *)(intptr_t)status;
		asm volatile ("syscall" : : "r" (sysno), "r" (stat));
	}
}

void arch_init_heap(void **region, size_t *sz) {
	register void *sysno asm ("rax") = (void *) SYS_MMAP;
	register void *addr asm ("rdi") = (void *) 0;
	register void *size asm ("rsi") = (void *) HEAP_SIZE;
	register void *prot asm ("rdx") = (void *) (PROT_READ | PROT_WRITE);
	register void *flags asm ("r10") = (void *) (MAP_SHARED | MAP_ANONYMOUS);
	register void *fd asm ("r8") = (void *) 0;
	register void *offset asm ("r9") = (void *) 0;
	register void *ret asm ("rax");
	asm volatile ("syscall" : "=r" (ret) : "r" (sysno), "r" (addr), "r" (size), "r" (prot), "r" (flags), "r" (fd), "r" (offset));
	if(ret == MAP_FAILED) arch_halt(_HALT_ERROR);
	*region = ret;
	*sz = HEAP_SIZE;
}

int main();
void _start() { main(); arch_halt(0); }
