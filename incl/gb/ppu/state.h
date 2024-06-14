#ifndef GB_PPU_STATE_H
#define GB_PPU_STATE_H
#include <stdalign.h>
#include <stdint.h>
#include "gb/mem/region.h"

enum {
	// DMG palettes are actually 3 bytes total, but the value is rounded up
	// to the nearest greater power of 2 for fast index multiplication.
	PPU_CGBPAL_SZ = 128, // bytes
	
	PPU_BGP = 2,
};

struct gb_ppu_state {
	// Immutable mid-line (all models):
	uint8_t* vram; // MEM_VRAM_SZ on non-CGB, double that on CGB
	uint8_t* oam; // MEM_OAM_SZ
	// Immutable mid-line on CGB,
	// can be modified mid-line on all other models:
	uint8_t* palette;
	// Can be modified mid-line:
	uint8_t lcdc;
	uint8_t scy;
	uint8_t scx;
	uint8_t wy;
	uint8_t wx;
	uint8_t mode;
}; // end struct gb_ppu_state

#endif // GB_PPU_STATE_H

