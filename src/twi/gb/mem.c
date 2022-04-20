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
#include <stdint.h>
#include <twi/std/assertf.h>
#include <twi/gb/log.h>
#include <twi/gb/mem.h>

//=======================================================================
//-----------------------------------------------------------------------
// External constant definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def TWI_GB_MEM_SZ_VRAM
//=======================================================================
const size_t TWI_GB_MEM_SZ_VRAM = 8192; // 8 KiB

//=======================================================================
//-----------------------------------------------------------------------
// External function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def twi_gb_mem_read8()
//=======================================================================
uint8_t
twi_gb_mem_read8(const struct twi_gb_mem* mem, uint16_t addr) {
	return 0; // TODO
} // end twi-gb_mem_read8()

//=======================================================================
// def twi_gb_mem_read_sector()
//=======================================================================
const uint8_t*
twi_gb_mem_read_sector(
		const struct twi_gb_mem* mem,
		enum twi_gb_mem_sector sector
) {
	twi_assert_notnull(mem);
	switch (sector) {
		case TWI_GB_MEM_SECTOR_VRAM0: return mem->vram0;
		case TWI_GB_MEM_SECTOR_OAM: return mem->oam;
		case TWI_GB_MEM_SECTOR_CTL: return mem->ctl;
		default:
			twi_assertf(0, "Invalid or unhandled sector provided (%d).", sector);
			return NULL;
	} // end switch (sector)
} // end twi_gb_mem_read_sector()

