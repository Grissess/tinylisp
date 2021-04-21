SECTIONS {
	.rodata.tl_init_ents : SUBALIGN(8) {
		_TL_INIT_ENTS_START = .;
		*(.tl_init_ents)
		_TL_INIT_ENTS_END = .;
	}
}
INSERT AFTER .text;
