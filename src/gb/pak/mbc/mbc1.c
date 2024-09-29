#include <assert.h>
#include <stdint.h>
#include "gb/pak/const.h"
#include "gb/pak/mbc/common.h"
#include "gb/pak/mbc/mbc1.h"
#include "gb/pak/typedef.h"

//=======================================================================
// def mbc_write8_rom_mbc1()
void
mbc_write8_mbc1_rom(
		struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint16_t addr, uint8_t val) {
	assert(addr < PAK_ROM_BANK_SIZE * 2);
	if (addr < 0x2000) { // RAM Enable
		//mbc_ram_enable(pak, // TODO: Need access to RAM map.
		// TODO: Probably want to combine MBC write functions into 1,
		//       if their mapping regions are really so co-dependent.
	}
} // end mbc_write8_rom_mbc1()
