CC = riscv64-unknown-elf-gcc
override CFLAGS += -mcmodel=medany $(call polyfill,busted_stdint)
override LDFLAGS += -T minilibc/arch/rv64g.ld
PLAT = RISCV
