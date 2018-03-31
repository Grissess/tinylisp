OBJ := builtin.o env.o eval.o interp.o object.o print.o read.o debug.o main.o
CFLAGS := -g -std=gnu99 -DDEBUG
LDFLAGS := 
DESTDIR := /usr/local/
CC := gcc

.PHONY: all clean

all: tl

clean:
	rm $(OBJ)

install: tl
	install -D -t $(DESTDIR)/bin/ $?

tl: $(OBJ)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
