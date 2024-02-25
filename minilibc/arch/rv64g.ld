SECTIONS {
	. = 0x80000000;
	all : {
		  *(begin)
		  *(.*)
		  heap_start = .;
	}
}
