CC = riscv64-unknown-elf-gcc
STATIC = 1
override CFLAGS += -march=rv32im -mabi=ilp32 $(call polyfill,busted_stdint)
override LDFLAGS += -T minilibc/arch/rv32im.ld
PLAT = RISCV
