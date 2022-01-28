#ifndef MINILIBC_ARCH_H
#define MINILIBC_ARCH_H

#define EOF ((int) -1)

#define STDIN 1
#define STDOUT 2
#define STDERR 3

#define _HALT_ERROR 0xff

#define _NORETURN __attribute__((noreturn))
#define WASM_IMPORT(nm) __attribute__((import_module("tl"),import_name(nm)))

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
void WASM_IMPORT("fflush") arch_fflush(unsigned long);
void WASM_IMPORT("fputc") arch_fputc(unsigned long, char);
int WASM_IMPORT("fgetc") arch_fgetc(unsigned long);
void _NORETURN WASM_IMPORT("halt") arch_halt(int);
void WASM_IMPORT("new_heap") arch_new_heap(size_t, void **, size_t *);
void WASM_IMPORT("release_heap") arch_release_heap(void *, size_t);
#pragma GCC diagnostic pop

#endif
