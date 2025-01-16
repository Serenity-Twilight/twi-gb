#include <assert.h>
#include <stdint.h>
#include "gb/pak/const.h"
#include "gb/pak/mbc/common.h"
#include "gb/pak/mbc/mbc1.h"
#include "gb/pak/typedef.h"

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL FUNCTION DECLARATIONS
//-----------------------------------------------------------------------
//=======================================================================
static void
set_bank_mode(
		struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint8_t* restrict ram_map,
		uint8_t new_bank_mode);
static void
set_lower_rom_bank(
		struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint8_t new_bank);
static void
set_upper_rom_bank(
		struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint8_t upper_bank_bits);

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL FUNCTION DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def mbc_w8_rom_mbc1()
void
mbc_w8_mbc1_rom(
		struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint8_t* restrict ram_map,
		uint16_t addr, uint8_t val) {
	assert(pak != NULL);
	assert(addr < PAK_ROM_BANK_SIZE * 2);
	static_assert(PAK_ROM_BANK_SIZE * 2 == 0x8000,
			"`PAK_ROM_BANK_SIZE` is not the value expected by `mbc_w8_mbc1_rom()`'s logic.");
	if (addr < 0x2000) { // 0x0000-0x1FFF: RAM Enable
		// Enable RAM if `val`'s lower 4 bits == 0xA, else disable:
		mbc_ram_enable(pak, ram_map, (val & 0xF) == 0xA);
	} else if (addr < 0x4000) { // 0x2000-0x3FFF: ROM Bank Number
		set_lower_rom_bank(pak, rom_map, val);
	} else if (addr < 0x6000) { // 0x4000-0x5FFF:
		// RAM Bank Number -or- Upper Bits of ROM Bank Number
		if (pak->rom_bank_count >= 64) // ROM >= 1 MiB
			set_upper_rom_bank(pak, rom_map, val);
		else if (pak->ram_bank_count == 4) { // RAM == 32 KiB
			val &= 0x3; // Only keep 2 LSB
			if (pak->bank_mode) // RAM remapping is enabled
				mbc_swap_ram_bank(pak, ram_map, val);
			else // RAM locked to bank 0, but save desired bank index
				pak->masked_ram_bank = val;
		} // If ROM and RAM are both too small to use this register, do nothing
	} else { // 0x6000-0x7FFF: Bank Mode Select
		set_bank_mode(pak, rom_map, ram_map, val);
	} // end address range identification
} // end mbc_w8_mbc1_rom()

//=======================================================================
// def mbc_w8_ram_mbc1()
void
mbc_w8_mbc1_ram(
		struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint8_t* restrict ram_map,
		uint16_t addr, uint8_t val) {
	mbc_ram_write(pak, ram_map, addr, val);
} // end mbc_w8_mbc1_ram()

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL FUNCTION DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// doc set_bank_mode()
// TODO
//=======================================================================
// def set_bank_mode()
static void
set_bank_mode(
		struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint8_t* restrict ram_map,
		uint8_t new_bank_mode) {
	assert(pak != NULL);
	assert(rom_map != NULL);
	assert(ram_map != NULL);

	// Bank mode setting determined by value of bit 0:
	if (new_bank_mode & 0x1) { // Enable advanced banking
		if (pak->bank_mode)
			return; // Already enabled. No change.
		pak->bank_mode = 1;
		if (pak->rom_bank_count >= 64) { // ROM size >= 1 MiB
			// Unlock ROM1 remapping:
			if (pak->rom_bank_curr >= 64) {
				// ROM bank bits 5-6 > 0, remap ROM1 to `bits 5-6 * 0x20`:
				mbc_swap_rom1_bank(
						pak, rom_map, ((pak->rom_bank_curr >> 5) & 0x3) * 0x20);
			} // else: Bank 0 is already the correct (and current) mapping
		} else if (pak->ram_bank_count == 4) { // RAM size == 32 KiB
			// Unlock RAM remapping:
			if (pak->masked_ram_bank > 0) {
				// Since advanced banking was locked, bank 0 is mapped to memory,
				// and `masked_ram_bank` reflects the desired, but not active,
				// RAM bank.
				mbc_swap_ram_bank(pak, ram_map, pak->masked_ram_bank);
			} // else: Bank 0 is already the correct (and current) mapping
		} // else: ROM and RAM are too small, no effect
	} else { // Disable advanced banking
		if (!(pak->bank_mode))
			return; // Already disabled. No change.
		pak->bank_mode = 0;
		if (pak->rom_bank_count >= 64) { // ROM size >= 1 MiB
			// Lock ROM1 mapping to bank 0:
			if (pak->rom_bank_curr >= 64)
				mbc_swap_rom1_bank(pak, rom_map, 0); // Map bank 0
			// else: ROM1 already mapped to bank 0
		} else if (pak->ram_bank_count == 4) { // RAM size == 32 KiB
			// Lock RAM mapping to bank 0:
			// Save current bank, in case advanced banking is re-enabled:
			pak->masked_ram_bank = pak->ram_bank_curr;
			if (pak->masked_ram_bank > 0)
				mbc_swap_ram_bank(pak, ram_map, 0); // Map bank 0
			// else: RAM already mapped to bank 0
		} // else: ROM and RAM are too small, no effect
	} // end ifelse (new_bank_mode)
} // end set_bank_mode()

//=======================================================================
// doc set_lower_rom_bank()
// TODO
//=======================================================================
// def set_lower_rom_bank()
static void
set_lower_rom_bank(
		struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint8_t new_bank_id) {
	assert(pak != NULL);
	// Mask down to lower 5 bits (top 3 bits are ignored):
	new_bank_id &= 0x1F; // 0001 1111
	// Block direct selection of bank 0, converting it to 1:
	if (new_bank_id == 0)
		new_bank_id = 1;

	if (pak->rom_bank_count <= 16) {
		// Mask out unused higher-order bits when pak has <=16 ROM banks
		// (for 16 banks, mask out bit 4; for 8 bits, mask out bits 3-4; etc)
		// This allows bank 0 to be selected by writing a non-zero value in
		// which all unmasked bits are 0. This fault exists on original hardware.
		// Note: `rom_bank_count` guaranteed to be a power of 2
		new_bank_id &= (pak->rom_bank_count - 1);
	} else if (pak->rom_bank_count > 32) {
		// For paks with >32 ROM banks, 2 additional higher-order bits are
		// utilized for ROM selection. These bits are set by writing to
		// 0x4000-0x5FFF, and thus are not overwritten and must be kept
		// when overwriting the lower 5 bits.
		new_bank_id |= (pak->rom_bank_curr & 0x60); // 0110 0000
	}

	mbc_swap_rom2_bank(pak, rom_map, new_bank_id);
} // end set_lower_rom_bank()

//=======================================================================
// doc set_upper_rom_bank()
// TODO
//=======================================================================
// def set_upper_rom_bank()
static void
set_upper_rom_bank(
		struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint8_t upper_bank_bits) {
	assert(pak != NULL);
	assert(pak->rom_bank_count >= 64);
	if (pak->rom_bank_count > 64)
		upper_bank_bits &= 0x3; // 2 MiB ROM: 2 upper bank bits
	else
		upper_bank_bits &= 0x1; // 1 MiB ROM: 1 upper bank bit
	// Overwrite previous upper bank bits:
	uint8_t rom2_bank = (upper_bank_bits << 5) | (pak->rom_bank_curr & 0x1F);
	if (rom2_bank == pak->rom_bank_curr)
		return; // No change to currently selected ROM2 bank.
	
	// If `pak->bank_mode` is set, then the ROM1 memory region is unlocked.
	// If unlocked, then this region is remapped based on the upper bank bits.
	if (pak->bank_mode)
		mbc_swap_rom1_bank(pak, rom_map, upper_bank_bits * 0x20);
	mbc_swap_rom2_bank(pak, rom_map, rom2_bank);
} // end set_upper_rom_bank()

