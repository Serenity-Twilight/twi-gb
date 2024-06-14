#include <assert.h>
#include <stdint.h>
#include "gb/mem.h"
#include "gb/pak.h"
#include "gb/pak/const.h"
#include "gb/pak/mbc/common.h"
#include "gb/pak/typedef.h"

//=======================================================================
// def mbc_sram_write()
void
mbc_sram_write(
		struct gb_mem* restrict mem,
		struct gb_pak* restrict pak,
		uint16_t addr, uint8_t val) {
	assert(mem != NULL);
	assert(pak != NULL);
	assert(addr >= MEM_B_SRAM && addr < MEM_E_SRAM);

	if (pak->ram_bank_count > 0) {
		assert(pak->ram != NULL);
		// Write to memory map's external RAM region:
		mem->map[addr] = val;
		// Write to pak's backing SRAM array:
		addr -= MEM_B_SRAM;
		((uint8_t*)pak->ram)[pak->ram_bank_curr * PAK_RAM_BANK_SIZE + addr] = val;
		pak->dirty_ram = 1;
	} // end if (pak->ram_bank_count > 0)
} // end mbc_sram_write()

