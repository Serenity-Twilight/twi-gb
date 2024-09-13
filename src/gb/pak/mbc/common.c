#include <assert.h>
#include <stdint.h>
#include "gb/pak/const.h"
#include "gb/pak/mbc/common.h"
#include "gb/pak/typedef.h"

//=======================================================================
// def mbc_ram_write()
void
mbc_ram_write(
		struct gb_pak* restrict pak,
		uint8_t* restrict ram_map,
		uint16_t addr, uint8_t val) {
	assert(pak != NULL);
	assert(addr >= MEM_B_SRAM && addr < MEM_E_SRAM);

	if (pak->ram_bank_count > 0) {
		assert(pak->ram != NULL);
		assert(pak->ram_bank_curr < pak->ram_bank_count);
		if (pak->ram_enabled) {
			addr -= MEM_B_SRAM;
			// Update copy of RAM owned by memory map:
			ram_map[addr] = val;
			// Update RAM owned by pak:
			((uint8_t*)pak->ram)[pak->ram_bank_curr * PAK_RAM_BANK_SIZE + addr] = val;
			pak->dirty_ram = 1;
		} // end if (pak->ram_enabled)
	} // end if (pak->ram_bank_count > 0)
} // end mbc_ram_write()

// TODO: Move ROM & RAM swap functions here!

