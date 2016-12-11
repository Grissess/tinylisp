OBJ := builtin.o env.o eval.o interp.o object.o print.o read.o debug.o main.o
CFLAGS := -g -std=gnu99 -DDEBUG
LDFLAGS := -lgc
CC := gcc

.PHONY: all


all: tl

tl: $(OBJ)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
