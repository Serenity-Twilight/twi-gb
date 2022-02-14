//=======================================================================
// TODO
// Written by Serenity Twilight
//=======================================================================
#ifndef TWI_GB_MEM_H
#define TWI_GB_MEM_H
#include <stdint.h>

//=======================================================================
// decl enum twi_gb_mem_ctl
// def enum twi_gb_mem_ctl
//
// Offsets of control registers within twi_gb_mem.ctl.
//=======================================================================
enum twi_gb_mem_ctl {
	TWI_GB_MEM_CTL_LCDC = 0x40, // LCD Control
	TWI_GB_MEM_CTL_STAT = 0x41, // LCD Status Information
	TWI_GB_MEM_CTL_SCY = 0x42, // Scroll Y TODO: More detail
	TWI_GB_MEM_CTL_SCX = 0x43, // Scroll X TODO: More detail
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
// TODO: This is currently a skeleton to get video running.
//       The rest will be implemented later.
//=======================================================================
struct twi_gb_mem {
	uint8_t vram[8192];
	uint8_t oam[160];
	uint8_t ctl[128];
};

//=======================================================================
// decl twi_gb_mem_read_sector()
// TODO
//=======================================================================

#endif // TWI_GB_MEM_H

