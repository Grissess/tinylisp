SECTIONS {
	.rodata.tl_bmcons : {
		_TL_BUILTIN_MODULES_START = .;
		*(.tl_bmcons)
		_TL_BUILTIN_MODULES_END = .;
	}
}
INSERT AFTER .text;
