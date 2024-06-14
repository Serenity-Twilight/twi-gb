#include <assert.h>
#include <stdint.h>
#include "gb/mem.h"
#include "gb/pak.h"
#include "gb/pak/mbc/common.h"
#include "gb/pak/mbc/none.h"
#include "gb/pak/typedef.h"

//=======================================================================
// def mbc_write8_rom_none()
void
mbc_write8_rom_none(
		struct gb_mem* restrict,
		struct gb_pak* restrict,
		uint16_t, uint8_t) {
	(void)0; // No MBC. Writes do nothing to the ROM.
} // end mbc_write8_rom_none()

//=======================================================================
// def mbc_write8_ram_none()
void
mbc_write8_ram_none(
		struct gb_mem* restrict mem,
		struct gb_pak* restrict pak,
		uint16_t addr, uint8_t val) {
	assert(pak != NULL);
	assert(pak->ram_bank_count <= 1);
	mbc_sram_write(mem, pak, addr, val);
} // end mbc_write8_ram_none()

