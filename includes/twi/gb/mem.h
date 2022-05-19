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
// TODO
//-----------------------------------------------------------------------
//=======================================================================
#ifndef TWI_GB_MEM_H
#define TWI_GB_MEM_H

#include <stddef.h>
#include <stdint.h>
#include <twi/gb/mode.h>

//=======================================================================
//-----------------------------------------------------------------------
// External type definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// decl enum twi_gb_mem_ctl
// def enum twi_gb_mem_ctl
//
// Offsets of control registers within twi_gb_mem.ctl.
// TODO: Not all ctl registers listed. This is for getting LCD running.
//=======================================================================
enum twi_gb_mem_ctl {
	TWI_GB_MEM_CTL_LCDC = 0x40, // LCD Control
	TWI_GB_MEM_CTL_STAT = 0x41, // LCD Status Information
	TWI_GB_MEM_CTL_SCY = 0x42, // Scroll Y
	TWI_GB_MEM_CTL_SCX = 0x43, // Scroll X
	TWI_GB_MEM_CTL_LY = 0x44, // LCDC Y coordinate
	TWI_GB_MEM_CTL_LYC = 0x45, // LY compare register
	TWI_GB_MEM_CTL_DMA = 0x46, // DMA transfer
	TWI_GB_MEM_CTL_BGP = 0x47, // BG palette data (DMG)
	TWI_GB_MEM_CTL_OBP0 = 0x48, // OBJ palette data 0 (DMG)
	TWI_GB_MEM_CTL_OBP1 = 0x49, // OBJ palette data 1 (DMG)
	TWI_GB_MEM_CTL_WY = 0x4A, // Window Y coordinate
	TWI_GB_MEM_CTL_WX = 0x4B, // Window X coordinate
}; // end enum twi_gb_mem_ctl

//=======================================================================
// decl enum twi_gb_mem_sector
// def enum twi_gb_mem_sector
//
// Internal value used for quickly identifying sectors in the memory map.
// TODO: Incomplete. This is a skeleton to get LCD working.
//=======================================================================
enum twi_gb_mem_sector {
	TWI_GB_MEM_SECTOR_VRAM, // Video Memory
	TWI_GB_MEM_SECTOR_OAM, // Object Attribute Memory
	TWI_GB_MEM_SECTOR_CTL, // Control Registers
};

//=======================================================================
// TODO: This is currently a skeleton to get video running.
//       The rest will be implemented later.
//=======================================================================
struct twi_gb_mem {
	_Alignas(max_align_t) uint8_t map[0x10000]; // 64 KiB
	const uint8_t* rom;
	uint8_t* vram;
	uint8_t* eram;
	uint8_t* wram;
	uint8_t rom_bank;
	uint8_t eram_bank;
};

//=======================================================================
//-----------------------------------------------------------------------
// External variable declarations
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// decl TWI_GB_MEM_SZ_VRAM
//
// The size of a single bank of VRAM.
// The combined size of both banks of VRAM available to CGB
// is equal to double this value.
//=======================================================================
extern const size_t TWI_GB_MEM_SZ_VRAM;

//=======================================================================
//-----------------------------------------------------------------------
// External function declarations
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// decl twi_gb_mem_init()
// TODO
//=======================================================================
int_fast8_t
twi_gb_mem_init(
		struct twi_gb_mem* restrict mem,
		const char* rom_path,
		const char* sram_path,
		enum twi_gb_mode preferred_mode
);

//=======================================================================
// decl twi_gb_mem_read8()
// TODO
//=======================================================================
uint8_t
twi_gb_mem_read8(const struct twi_gb_mem* restrict mem, uint16_t addr);

//=======================================================================
// decl twi_gb_mem_read16()
// TODO
//=======================================================================
uint16_t
twi_gb_mem_read16(const struct twi_gb_mem* restrict mem, uint16_t addr);

//=======================================================================
// decl twi_gb_mem_write8()
// TODO
//=======================================================================
void
twi_gb_mem_write8(struct twi_gb_mem* restrict mem, uint16_t addr, uint8_t val);

//=======================================================================
// decl twi_gb_mem_write16()
// TODO
//=======================================================================
void twi_gb_mem_write16(struct twi_gb_mem* restrict mem, uint16_t addr, uint16_t val);

//=======================================================================
// decl twi_gb_mem_read_sector()
//
// Provides a readonly pointer to the sector of the memory map
// indicated by `sector`.
//
// The sizes of different memory map sectors vary.
// Adhere to the definition of `twi_gb_mem_sector` for size information
// on each sector. Attempting to read beyond the end of any particlar
// sector returned by this function will result in undefined behavior.
//
// Behavior is undefined if `mem` does not point to a valid
// `twi_gb_mem` object, or if the value of `sector` does not equate to
// one of the valid enumerated values of `twi_gb_mem_sector`.
//-----------------------------------------------------------------------
// Parameters:
// * mem: The memory map being accessed.
// * sector: The sector of memory to access within the memory map.
//
// Returns: A constant pointer to the specified memory sector within
// the provided memory map.
//=======================================================================
const uint8_t*
twi_gb_mem_read_sector(
		const struct twi_gb_mem* restrict mem,
		enum twi_gb_mem_sector sector
);

//=======================================================================
// decl twi_gb_mem_mode()
//
// Indicates the operating mode of the emulated system.
// *  0: DMG mode
// * !0: CGB mode
//
// Behavior is undefined if `mem` does not point to an initialized,
// undestroyed twi-gb-mem object.
//-----------------------------------------------------------------------
// Parameters:
// * mem: Pointer to an initialized, undestroyed twi-gb-mem object.
//
// Returns: The mode that the provided twi-gb-mem object is running in.
//=======================================================================
enum twi_gb_mode
twi_gb_mem_mode(const struct twi_gb_mem* restrict mem);

#endif // TWI_GB_MEM_H

