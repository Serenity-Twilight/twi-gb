//=======================================================================
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
#include <twi/std/assert.h>
#include <twi/gb/common.h>
#include <twi/gb/lcd.h>
#include <twi/gb/log.h>
#include <twi/gb/mem.h>
#include <twi/gb/sdl.h>

//=======================================================================
//-----------------------------------------------------------------------
// Internal macro definitions
//-----------------------------------------------------------------------
//=======================================================================
// LCDC bitmasks
#define BGDISPLAY 0x01 // 0: BG off (DMG only), 1: BG on
#define OBJDISPLAY 0x02 // 0: OBJs off, 1: OBJs on
#define OBJBLOCK 0x04 // 0: OBJs are 8x8, 1: OBJs are 8x16
#define BGCODEAREA 0x08 // 0: 0x9800-0x9BFF, 1: 0x9C00-0x9FFF
#define BGCHARDATA 0x10 // 0: 0x8800-0x97FF, 1: 0x8000-0x8FFF
#define WINDISPLAY 0x20 // 0: Window off, 1: Window on
#define WINCODEAREA 0x40 // 0: 0x9800-0x9BFF, 1: 0x9C00-0x9FFF
#define LCDON 0x80 // 0: LCD off, 1: LCD on

// Offsets for BG character code selection (tiles pointing to graphics).
// Offsets are from the start of sector VRAM0 and VRAM1 (0x8000).
#define WINCODEAREA_SIZE 1024 // bytes
#define BGCODEAREA_POS0 0x1800 // 0x9800-0x9BFF
#define BGCODEAREA_POS1 0x1C00 // 0x9C00-0x9FFF

// Offsets for BG character code data (actual graphics data).
// Offsets are from the start of sectors VRAM0 and VRAM1 (0x8000).
#define BGCHARDATA_SIZE 4096 // bytes
#define BGCHARDATA_POS0 0x0800 // 0x8800-0x97FF
#define BGCHARDATA_POS1 0x0000 // 0x8000-0x8FFF

// Offsets for window character code selection (tiles pointing to graphics).
// Offsets are from the start of sector VRAM0 and VRAM1 (0x8000).
#define WINCODEAREA_SIZE 1024 // bytes
#define WINCODEAREA_POS0 0x1800 // 0x9800-0x9BFF
#define WINCODEAREA_POS1 0x1C00 // 0x9C00-0x9FFF

// Offsets for OAM registers, treated as uint8_t[4].
#define OBJY 0
#define OBJX 1
#define OBJCODE 2
#define OBJATRB 3

// LCD screen dimensions (in pixels)
#define SCR_WIDTH 160
#define SCR_HEIGHT 144

// System-defined color codes
#define COLOR_WHITE 0xFFFFFFFF

//=======================================================================
//-----------------------------------------------------------------------
// Internal function declarations
//-----------------------------------------------------------------------
//=======================================================================
static void
get_palette_data(uint32_t[], const struct twi_gb_mem*, enum twi_gb_mode);

//=======================================================================
//-----------------------------------------------------------------------
// External function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def twi_gb_lcd_draw()
//=======================================================================
uint_fast8_t
twi_gb_lcd_draw(
		struct twi_gb_lcd* lcd,
		const struct twi_gb_mem* mem,
		enum twi_gb_mode mode
) {
	twi_assert_notnull(lcd);
	twi_assert_notnull(lcd->backend);
	twi_assert_notnull(mem);

	// Fetch draw destination.
	static const uint8_t px_width = 160;
	static const uint8_t px_height = 144;
	uint32_t* pixels = twi_gb_sdlvid_get_drawing_area(lcd->backend);
	if (pixels == NULL) {
		LOGE("Failed to execute draw: Backend did not provide a drawing target.");
		return 1;
	}

	// Fetch memory sectors containing draw source & palettes.
	const uint8_t* mem_vram0 = twi_gb_mem_read_sector(mem, TWI_GB_MEM_SECTOR_VRAM0);
	const uint8_t* mem_oam = twi_gb_mem_read_sector(mem, TWI_GB_MEM_SECTOR_OAM);
	const uint8_t* mem_ctl = twi_gb_mem_read_sector(mem, TWI_GB_MEM_SECTOR_CTL);

	// Map palettes to colors.
	uint8_t palettes[3]; // 0: BG palette, 1: OBJ0 palette, 2: OBJ1 palette
	get_palette_data(palettes, mem_ctl, mode);

	// TODO: Call draw_bg()
} // end twi_gb_lcd_draw()

//=======================================================================
//-----------------------------------------------------------------------
// Internal function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def compare_objs_dmg()
//
// Comparison function for sorting OBJs in preparation for drawing.
// An array of OBJs sorted using this comparison function should be
// traversed in order of ascending offset.
//
// This function is designed for compatibility with cstdlib's qsort().
//
// The priority rules for overlapping OBJs on DMG is as follows:
// 1. The OBJ with the lower x-coordinate takes priority (overlaps).
// 2. If the OBJ x-coordinates match, the OBJ with the lower character
//    code takes priority (overlaps).
//-----------------------------------------------------------------------
// Parameters:
// * lobj: A 32-bit object attribute memory value.
// * robj: A different 32-bit object attribute memory value.
//
// Returns:
// If `robj` should be drawn over `lobj`, then this function returns <0.
// If `lobj` should be drawn over `robj`, then this function returns >0.
// Otherwise, it returns 0 (matching x-coordinates and character codes).
//=======================================================================
static int
compare_objs_dmg(const void* lobj, const void* robj) {
	// Objects are packs of 4 8-bit values each.
	const uint8_t* lhs = (uint8_t*)a;
	const uint8_t* rhs = (uint8_t*)b;

	// Lowest x-coordinate is drawn last (overlaps).
	int diff = (int)(rhs[OBJX]) - (int)(lhs[OBJX]);
	if (diff != 0)
		return diff;
	// If x-coordinate matches, lowest object code is drawn last.
	return (int)(rhs[OBJCODE]) - (int)(lhs[OBJCODE]);
} // end compare_objs_dmg()

//=======================================================================
// def draw_bg()
// TODO
//=======================================================================
static void
draw_bg(
		uint32_t* dest,
		const uint32_t[][4] palettes,
		const uint8_t* vram0,
		const uint8_t* vram1,
		uint8_t lcdc,
		uint8_t start_line,
		uint8_t num_lines
) {
	// Assert arguments' validity.
	twi_assert_notnull(dest);
	twi_assert_notnull(palettes); // Decays to pointer.
	twi_assert_notnull(vram0);
	twi_assert_lti(start_line, SCR_HEIGHT);
	uint8_t end_line = start_line + num_lines;
	twi_assert_lteqi(end_line, SCR_HEIGHT);

	// If background is disabled by LCD controller, draw white to all
	// pixels of all lines to be drawn to.
	if (!(lcdc & BGDISPLAY)) {
		uint_fast16_t end_index = end_line * SCR_WIDTH;
		for (uint_fast16_t i = start_line * SCR_WIDTH; i < end_index; ++i)
			dest[i] = COLOR_WHITE;
		return;
	} // end if background is disabled.
} // end draw_bg()

//=======================================================================
// def get_palette_data()
// TODO
//=======================================================================
static void
get_palette_data(
		uint8_t palettes[],
		const uint8_t* mem_ctl,
		enum twi_gb_mode mode
) {
	twi_assert_notnull(mem);

	switch (mode) {
		case TWI_GB_MODE_DMG:
			palettes[0] = mem_ctl[TWI_GB_MEM_CTL_BGP];
			palettes[1] = mem_ctl[TWI_GB_MEM_CTL_OBP0];
			palettes[2] = mem_ctl[TWI_GB_MEM_CTL_OBP1];
			break;
		case TWI_GB_MODE_CGB:
			twi_assertf(0, "CGB support not yet implemented.");
			break;
		default: // Kill process
			twi_assertf(0, "An invalid twi_gb_mode was provided (%d).", mode);
	} // end switch (mode)
} // end get_palette_data()

