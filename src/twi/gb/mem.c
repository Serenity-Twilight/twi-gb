//=======================================================================
//-----------------------------------------------------------------------
// src/twi/gb/mem.c
// Written by Serenity Twilight
//-----------------------------------------------------------------------
//=======================================================================
#include <stdint.h>
#include <twi/std/assert.h>
#include <twi/gb/log.h>
#include <twi/gb/mem.h>

//=======================================================================
//-----------------------------------------------------------------------
// External function definitions
//-----------------------------------------------------------------------
//=======================================================================

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
		case VRAM0: return mem->vram0;
		case OAM: return mem->oam;
		case CTL: return mem->ctl;
		default:
			twi_assertf(0, "Invalid or unhandled sector provided (%d).", sector);
			return NULL;
	} // end switch (sector)
} // end twi_gb_mem_read_sector()

