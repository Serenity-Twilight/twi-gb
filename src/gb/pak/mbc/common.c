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
	assert(addr < MEM_SZ_SRAM);

	if (pak->ram_bank_count > 0) {
		assert(pak->ram != NULL);
		assert(pak->ram_bank_curr < pak->ram_bank_count);
		if (pak->ram_enabled) {
			// Update copy of RAM owned by memory map:
			ram_map[addr] = val;
			// Update RAM owned by pak:
			((uint8_t*)pak->ram)[pak->ram_bank_curr * PAK_RAM_BANK_SIZE + addr] = val;
			pak->dirty_ram = 1;
		} // end if (pak->ram_enabled)
	} // end if (pak->ram_bank_count > 0)
} // end mbc_ram_write()

//=======================================================================
// def mbc_swap_rom_bank()
void
mbc_swap_rom_bank(
		const struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint16_t new_bank_id) {
	assert(pak != NULL);
	assert(pak->rom != NULL);
	assert(pak->rom_bank_count >= 2);
	assert(rom_map != NULL);

	// Truncate values that are greater than the total number of banks:
	new_bank_id %= pak->rom_bank_count;
	if (new_bank_id == pak->rom_bank_curr)
		return; // New bank == old bank, do nothing
	memcpy(rom_map + MEM_B_ROM2,
			pak->rom + new_bank_id * PAK_ROM_BANK_SIZE,
			PAK_ROM_BANK_SIZE);
	pak->rom_bank_curr = new_bank_id;
} // end mbc_swap_rom_bank()

//=======================================================================
// def mbc_swap_ram_bank()
void
mbc_swap_ram_bank(
		const struct gb_pak* restrict pak,
		uint8_t* restrict ram_map,
		uint8_t new_bank_id) {
	assert(pak != NULL);
	assert(ram_map != NULL);

	if (pak->ram != NULL) {
		assert(pak->ram_bank_count > 0);
		// Truncate values that are greater than the total number of banks:
		new_bank_id %= pak->ram_bank_count;
		if (new_bank_id == pak->ram_bank_curr)
			return; // New bank == old bank, do nothing
		memcpy(ram_map, pak->ram + new_bank_id * PAK_RAM_BANK_SIZE, PAK_RAM_BANK_SIZE);
		pak->ram_bank_curr = new_bank_id;
	} // else no external RAM is present, do nothing
} // end mbc_swap_ram_bank()

