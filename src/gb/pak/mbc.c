#include "gb/pak/mbc.h"
#include "gb/pak/mbc/none.h"

const struct mbc_write8_pair mbc_write8[] = {
	{ .rom=mbc_write8_rom_none, .ram=mbc_write8_ram_none }, // NONE
}; // end mbc_write8[]

