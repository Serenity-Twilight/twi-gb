#include <assert.h>
#include <stdint.h>
#include "gb/pak/mbc/common.h"
#include "gb/pak/mbc/none.h"
#include "gb/pak/typedef.h"

//=======================================================================
// def mbc_write8_none_rom()
void
mbc_write8_none_rom(
		struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint16_t addr, uint8_t val) {
	(void)0; // No MBC. Writes do nothing to the ROM.
} // end mbc_write8_none_rom()

//=======================================================================
// def mbc_write8_none_ram()
void
mbc_write8_none_ram(
		struct gb_pak* restrict pak,
		uint8_t* restrict ram_map,
		uint16_t addr, uint8_t val) {
	assert(pak != NULL);
	assert(pak->ram_bank_count <= 1);
	mbc_ram_write(pak, ram_map, addr, val);
} // end mbc_write8_none_ram()

