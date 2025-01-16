#include <assert.h>
#include <stdint.h>
#include "gb/pak/const.h"
#include "gb/pak/mbc/common.h"
#include "gb/pak/typedef.h"

//=======================================================================
// def mbc_ram_enable()
void
mbc_ram_enable(
		struct gb_pak* restrict pak,
		uint8_t* restrict ram_map,
		uint_fast8_t enable) {
	assert(pak != NULL);
	if (pak->ram_bank_count == 0)
		return; // This pak has no RAM.
	assert(pak->ram_bank_curr < pak->ram_bank_count);

	if (enable) {
		if (!(pak->ram_enabled)) { // currently disabled
			// Copy currently selected RAM bank to `ram_map`:
			memcpy(ram_map, pak->ram + pak->ram_bank_curr * PAK_RAM_BANK_SIZE, PAK_RAM_BANK_SIZE);
			pak->ram_enabled = 1;
		}
	} else { // disable
		if (pak->ram_enabled) { // currently enabled
			// Copy 0xFF into `ram_map`, simulating a failed read from disabled RAM:
			memset(ram_map, 0xFF, PAK_RAM_BANK_SIZE);
			pak->ram_enabled = 0;
		}
	} // end ifelse (enable)
} // end mbc_ram_enable()

//=======================================================================
// def mbc_ram_write()
void
mbc_ram_write(
		struct gb_pak* restrict pak,
		uint8_t* restrict ram_map,
		uint16_t addr, uint8_t val) {
	assert(pak != NULL);
	assert(ram_map != NULL);
	assert(addr < PAK_RAM_BANK_SIZE);

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
// def mbc_swap_rom1_bank()
void
mbc_swap_rom1_bank(
		const struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint16_t new_bank_id) {
	assert(pak != NULL);
	assert(pak->rom != NULL);
	assert(rom_map != NULL);
	assert(new_bank_id < pak->rom_bank_count);

	// `gb_pak` contains no members to track the currently selected
	// ROM1 bank, so this function must assume that the caller has
	// greater context to deem this operation necessary.
	memcpy(rom_map,
			pak->rom + new_bank_id * PAK_ROM_BANK_SIZE,
			PAK_ROM_BANK_SIZE);
} // end mbc_swap_rom1_bank()

//=======================================================================
// def mbc_swap_rom2_bank()
void
mbc_swap_rom2_bank(
		struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint16_t new_bank_id) {
	assert(pak != NULL);
	assert(pak->rom != NULL);
	assert(rom_map != NULL);
	assert(new_bank_id < pak->rom_bank_count);

	if (new_bank_id == pak->rom_bank_curr)
		return; // New bank == old bank, do nothing
	memcpy(rom_map + PAK_ROM_BANK_SIZE,
			pak->rom + new_bank_id * PAK_ROM_BANK_SIZE,
			PAK_ROM_BANK_SIZE);
	pak->rom_bank_curr = new_bank_id;
} // end mbc_swap_rom2_bank()

//=======================================================================
// def mbc_swap_ram_bank()
void
mbc_swap_ram_bank(
		struct gb_pak* restrict pak,
		uint8_t* restrict ram_map,
		uint8_t new_bank_id) {
	assert(pak != NULL);
	assert(ram_map != NULL);

	if (pak->ram != NULL) {
		assert(pak->ram_bank_count > 0);
		assert(new_bank_id < pak->ram_bank_count);
		if (new_bank_id == pak->ram_bank_curr)
			return; // New bank == old bank, do nothing
		memcpy(ram_map, pak->ram + new_bank_id * PAK_RAM_BANK_SIZE, PAK_RAM_BANK_SIZE);
		pak->ram_bank_curr = new_bank_id;
	} // else no external RAM is present, do nothing
} // end mbc_swap_ram_bank()

