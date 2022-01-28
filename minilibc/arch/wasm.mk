override LDFLAGS += -Wl,--no-entry -Wl,--allow-undefined -Wl,--export-all -Wl,--no-gc-sections
override CFLAGS += --target=wasm32
CC = clang  # allow override
INTERPRETER = tl.wasm
