override LDFLAGS += -Wl,--no-entry -Wl,--allow-undefined -Wl,--export-all -Wl,--no-gc-sections
override CFLAGS += --target=wasm32 -DPTR_LSB_AVAILABLE=0
CC = clang  # allow override
INTERPRETER = tl.wasm
PLAT = WASM
