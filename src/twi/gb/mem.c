//======================================================================
//-----------------------------------------------------------------------
// Copyright 2022 Serenity Twilight
//
// This file is part of twi-gb.
//
// twi-gb is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// twi-gb is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with twi-gb. If not, see <https://www.gnu.org/licenses/>.
//-----------------------------------------------------------------------
//=======================================================================
#ifndef TWI_GB_MEM_C
#define TWI_GB_MEM_C

#include <stdint.h>
#include <stdlib.h>
#include <twi/std/assertf.h>
#include <twi/gb/log.h>
#include <twi/gb/mode.h>
#include <twi/gb/mem.h>

//=======================================================================
//-----------------------------------------------------------------------
// External constant definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// Memory block sizes
//=======================================================================
const size_t TWI_GB_MEM_SZ_VRAM = 8192; // 8 KiB, x2 on CGB
const size_t TWI_GB_MEM_SZ_WRAM_DMG = 8192; // 8 KiB
const size_t TWI_GB_MEM_SZ_WRAM_CGB = TWI_GB_MEM_SZ_WRAM_DMG * 4; // 32 KiB

//=======================================================================
//-----------------------------------------------------------------------
// Internal constant definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// Memory map sector start addresses
//=======================================================================
static const uint_fast16_t FX_ROM_BEGIN = 0x0000;
static const uint_fast16_t SW_ROM_BEGIN = 0x4000;
static const uint_fast16_t VRAM_BEGIN = 0x8000;
static const uint_fast16_t ERAM_BEGIN = 0xA000;
static const uint_fast16_t FX_WRAM_BEGIN = 0xC000;
static const uint_fast16_t SW_WRAM_BEGIN = 0xD000;
static const uint_fast16_t ECHO_RAM_BEGIN = 0xE000;
static const uint_fast16_t OAM_BEGIN = 0xFE00;
static const uint_fast16_t CTL_BEGIN = 0xFF00;
static const uint_fast16_t STACK_BEGIN = 0xFF80;

//=======================================================================
// Memory map sector end addresses
//=======================================================================
static const uint_fast16_t FX_ROM_END = SW_ROM_BEGIN;
static const uint_fast16_t SW_ROM_END = VRAM_BEGIN;
static const uint_fast16_t VRAM_END = ERAM_BEGIN;
static const uint_fast16_t ERAM_END = FX_WRAM_BEGIN;
static const uint_fast16_t FX_WRAM_END = SW_WRAM_BEGIN;
static const uint_fast16_t SW_WRAM_END = ECHO_RAM_BEGIN;
static const uint_fast16_t ECHO_RAM_END = OAM_BEGIN;
static const uint_fast16_t OAM_END = 0xFEA0;
static const uint_fast16_t CTL_END = STACK_BEGIN;
static const uint_fast16_t STACK_END = 0xFFFF;

//=======================================================================
// Address of Interrupt Enable register
//=======================================================================
static const uint_fast16_t IE_ADDR = 0xFFFF;

//=======================================================================
// Miscellaneous
//=======================================================================
// Number of bytes between start of WRAM and start of echo RAM.
static const uint_fast16_t ECHO_DIFF = ECHO_RAM_BEGIN - FX_WRAM_BEGIN;

//=======================================================================
//-----------------------------------------------------------------------
// External function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def twi_gb_mem_init()
//=======================================================================
int_fast8_t
twi_gb_mem_init(
		struct twi_gb_mem* restrict mem,
		const char* rom_path,
		const char* sram_path,
		enum twi_gb_mode preferred_mode
) {
	twi_assert_notnull(mem);

	mem->rom = NULL; // TODO: Load ROM
	mem->rom_bank = 0;
	mem->eram = NULL; // TODO: Load SRAM
	mem->eram_bank = 0;
	
	// TODO: Preferred mode should be superceded by ROM needs.
	// i.e. If the ROM loaded requires CGB, then always run in CGB mode.
	if (preferred_mode == TWI_GB_MODE_CGB) {
		// Allocated internal memory for VRAM and RAM.
		// CGB has x2 VRAM and x4 RAM of DMG.
		mem->vram = malloc(TWI_GB_MEM_SZ_VRAM * 2 + TWI_GB_MEM_SZ_WRAM_CGB);
		if (!(mem->vram))
			return 1;
		mem->wram = mem->vram + TWI_GB_MEM_SZ_VRAM * 2;
	} else {
		// Both fit entirely into the memory map on DMG.
		mem->vram = NULL;
		mem->wram = NULL;
	} // end if/else CGB/DMG mode

	return 0;
} // end twi_gb_mem_init()

//=======================================================================
// def twi_gb_mem_read8()
//=======================================================================
uint8_t
twi_gb_mem_read8(
		const struct twi_gb_mem* restrict mem,
		uint16_t addr
) {
	twi_assert_notnull(mem);
	return mem->map[addr];
} // end twi_gb_mem_read8()

//=======================================================================
// def twi_gb_mem_write8()
//=======================================================================
void
twi_gb_mem_write8(
		struct twi_gb_mem* restrict mem,
		uint16_t addr,
		uint8_t val
) {
	twi_assert_notnull(mem);
	switch (addr / 0x1000) { // Index by highest-order nibble.
		case 0x8: case 0x9:
			break; // TODO: VRAM
		case 0xA: case 0xB:
			break; // TODO: ERAM
		case 0xC: case 0xD: // WRAM: Write to WRAM and ECHO RAM. Writethrough on CGB.
			mem->map[addr] = val;
			if (addr < (ECHO_RAM_END - ECHO_DIFF))
				mem->map[addr + ECHO_DIFF] = val;
			if (mem->wram != NULL) // Writethrough
				mem->wram[(addr - FX_WRAM_BEGIN) + (TWI_GB_MEM_SZ_WRAM_DMG * 0)] = val; // TODO: Replace 0 with WRAM bank
			break; // TODO: WRAM
		case 0xE: // ECHO RAM: Write to ECHO RAM and WRAM. Writethrough on CGB.
			mem->map[addr] = val;
			mem->map[addr - ECHO_DIFF] = val;
			if (mem->wram != NULL) // Writethrough
				mem->wram[(addr - FX_WRAM_BEGIN) + (TWI_GB_MEM_SZ_WRAM_DMG * 0)] = val; // TODO: Replace 0 with WRAM bank
			break; // TODO: Echo RAM
		case 0xF:
			switch (addr / 0x100) { // Index nibble 0xF by 2nd-highest order nibble.
				case 0xFE:
					break; // TODO: OAM
				case 0xFF:
					break; // TODO: 0xFF stuff
				default: // Echo RAM: Write to ECHO RAM and WRAM. Writethrough on CGB.
					mem->map[addr] = val;
					mem->map[addr - ECHO_DIFF] = val;
					if (mem->wram != NULL) // Writethrough
						mem->wram[(addr - FX_WRAM_BEGIN) + (TWI_GB_MEM_SZ_WRAM_DMG * 0)] = val; // TODO: Replace 0 with WRAM bank
					break;
			}
			break;
		default:
			break; // TODO: MBC
	}
} // end twi_gb_mem_write8()

//=======================================================================
// def twi_gb_mem_read_sector()
//=======================================================================
const uint8_t*
twi_gb_mem_read_sector(
		const struct twi_gb_mem* restrict mem,
		enum twi_gb_mem_sector sector
) {
	twi_assert_notnull(mem);
	switch (sector) {
		case TWI_GB_MEM_SECTOR_VRAM:
			return (mem->vram != NULL ? mem->vram : &(mem->map[VRAM_BEGIN]));
		case TWI_GB_MEM_SECTOR_OAM: return &(mem->map[OAM_BEGIN]);
		case TWI_GB_MEM_SECTOR_CTL: return &(mem->map[CTL_BEGIN]);
		default:
			twi_assertf(0, "Invalid or unhandled sector provided (%d).", sector);
			return NULL;
	} // end switch (sector)
} // end twi_gb_mem_read_sector()

//=======================================================================
// def twi_gb_mem_mode()
//=======================================================================
enum twi_gb_mode
twi_gb_mem_mode(const struct twi_gb_mem* restrict mem) {
	twi_assert_notnull(mem);
	// The VRAM and WRAM swap buffers are NULL in DMG mode.
	return (mem->wram != NULL ? TWI_GB_MODE_CGB : TWI_GB_MODE_DMG);
} // end twi_gb_mem_mode()

#endif // TWI_GB_MEM_C

