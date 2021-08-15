#include "assert.h"
#include "stdio.h"
#include "arch.h"

int assertion_failed(const char *s, const long i) {
	fprintf(stderr, "Assertion failed: \"%s\" %ld\nHalt.\n", s, i);
	while(1) arch_halt(_HALT_ERROR);
	return 0;  /* Never reached */
}
