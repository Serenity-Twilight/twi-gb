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

// System-defined color codes
#define COLOR_WHITE 0xFFFFFFFF

//=======================================================================
//-----------------------------------------------------------------------
// Internal type definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// decl struct twi_gb_ppu_env
// def struct twi_gb_ppu_env
// TODO
//=======================================================================
struct twi_gb_ppu_env {
	const uint32_t** palettes;
	const uint8_t* bg_gfx[4];
	const uint8_t* win_gfx[4];
	const uint8_t* obj_gfx[2];
	const uint8_t* bg_codes;
	const uint8_t* bg_attrbs;
	const uint8_t* oam;
	const uint8_t* ctl;
}; // end struct twi_gb_ppu_env

//=======================================================================
// decl struct twi_gb_ppu_line
// def struct twi_gb_ppu_line
// 
// "What the heck is this?"
// This structure stores computed values used in drawing background
// (which includes window) lines. The reason for its existence is to
// allow the caller of the line drawing operation to perform these
// computations once so that the line drawing operation doesn't have to
// compute these values every single time it runs.
//
// Normally, this could be argued as unnecessary, but the background
// line drawing function must run 144 times per frame at 60 frames
// per second. It's hot enough to justify some micro-optimizations.
//-----------------------------------------------------------------------
// Members:
// * left_begin_x
//   X-offset, from 0-7 inclusive, to begin drawing the first tile.
// * left_end_x
//   X-offset, from 0-8 inclusive, at which to stop drawing the first tile.
//   For use when the total line length is less than the width of the
//   right side of the leftmost tile.
// * num_middle_tiles
//   The number of tiles between the leftmost and rightmost tiles.
//   These tiles will have their selected line drawn in their entirety.
// * right_end_x
//   X-offset, from 0-8 inclusive, at which to stop drawing the final tile.
//=======================================================================
struct twi_gb_ppu_line {
	uint_fast8_t left_begin_x;
	uint_fast8_t left_end_x;
	uint_fast8_t num_middle_tiles;
	uint_fast8_t right_end_x;
}; // end struct twi_gb_ppu_lin

//=======================================================================
// decl struct twi_gb_ppu_tile
// def struct twi_gb_ppu_tile
//=======================================================================
struct twi_gb_ppu_tile {
	uint_fast8_t y_begin;
	uint_fast8_t y_end;
	int_fast8_t x_shift_begin;
	int_fast8_t x_shift_end;
}; // end struct twi_gb_ppu_tile

//=======================================================================
//-----------------------------------------------------------------------
// Internal constants
//-----------------------------------------------------------------------
//=======================================================================

// The size, in bytes, of a single background tile_data region.
static const uint_fast8_t BG_TILE_DATA_SIZE = 0x80;

// The length of any side of a single character tile, in pixels.
static const uint_fast8_t TILE_PX_LENGTH = 8;

// The number of pixels/tiles any side of the entire background has along it.
static const uint_fast16_t BG_PX_LENGTH = 256;
static const uint_fast8_t BG_TILE_LENGTH = 32;

// The number of pixels/tiles wide and tall the LCD screen is.
static const uint_fast8_t SCREEN_PX_WIDTH = 160;
static const uint_fast8_t SCREEN_PX_HEIGHT = 144;
static const uint_fast8_t SCREEN_TILE_WIDTH = 20;
static const uint_fast8_t SCREEN_TILE_HEIGHT = 18;

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
		uint_fast8_t begin_line,
		uint_fast8_t end_line,
		const struct twi_gb_ppu_env* env,
		uint_fast8_t priority
) {
	//---Begin argument validation---
	twi_assert_notnull(dest);
	twi_assert_lt(begin_line, end_line);
	twi_assert_lteqi(end_line, SCREEN_PX_HEIGHT);
	twi_assert_notnull(env);
	twi_assert_notnull(env->palettes);
	twi_assert_notnull(env->bg_gfx[0]);
	twi_assert_notnull(env->bg_gfx[1]);
	twi_assert_notnull(env->win_gfx[0]);
	twi_assert_notnull(env->win_gfx[1]);
	if (env->bg_attrbs != NULL) {
		// CGB mode
		twi_assert_notnull(bg_gfx[2]);
		twi_assert_notnull(bg_gfx[3]);
		twi_assert_notnull(win_gfx[2]);
		twi_assert_notnull(win_gfx[3]);
	}
	twi_assert_notnull(env->bg_codes);
	twi_assert_notnull(ctl);
	//---End argument validation---
	
	uint32_t* curr_px = (dest + (begin_line * SCREEN_PX_WIDTH));
	// Draw color 0b00 for all pixels of selected lines if BG is off.
	// TODO: Need to account for window event if BG is off!
	if (!(env->ctl[TWI_GB_MEM_CTL_LCDC] & BGDISPLAY)) {
		uint_fast16_t curr_px = start_line * SCREEN_PX_WIDTH;
		uint_fast16_t end_px = end_line * SCREEN_PX_WIDTH;
		while (curr_px < end_px)
			dest[curr_px++] = COLOR_WHITE; // TODO: Make color customizable.
		return;
	} // end if background is disabled.

	uint_fast8_t wy; // Window Y coordinate
	if (env->ctl[TWI_GB_MEM_CTL_LCDC] & WINDISPLAY) // Window on
		wy = env->ctl[TWI_GB_MEM_CTL_WY];
	else // Window off
		wy = SCREEN_PX_HEIGHT; // bottom of the screen

	//--------------------------
	// Draw lines before window
	//--------------------------
	uint_fast8_t tile_x_offset = (env->ctl[TWI_GB_MEM_CTL_SCX] % TILE_PX_LENGTH);
	struct twi_gb_ppu_line bg_lines = {.left_begin_x = tile_x_offset};
	if (begin_line < wy) {
		bg_lines.left_end_x = TILE_PX_LENGTH;
		bg_lines.num_middle_tiles = (SCREEN_TILE_WIDTH-1);
		bg_lines.right_end_x = tile_x_offset;
		uint32_t* end_px = 
	}
	uint_fast8_t curr_px = start_line
	
} // end draw_bg()

//=======================================================================
// def draw_bg_line_dmg()
// TODO
//=======================================================================
static inline uint_fast8_t
draw_bg_line_dmg(
		uint32_t* dest,
		const uint32_t palette[4],
		const uint8_t* gfx[2],
		const uint8_t* line_codes,
		uint_fast8_t line_codes_offset,
		const struct twi_gb_ppu_line* x,
		uint_fast8_t tile_y_offset
) {
	//--- Begin argument validation ---
	// NOTE: This code may be too hot for these tests long-term.
	twi_assert_notnull(dest);
	twi_assert_notnull(palette);
	twi_assert_notnull(gfx[0]);
	twi_assert_notnull(gfx[1]);
	twi_assert_notnull(line_codes);
	twi_assert_lt(line_codes_offset, BG_TILE_LENGTH);
	twi_assert_notnull(x);
	twi_assert_lt(x->left_begin_x, TILE_PX_LENGTH);
	twi_assert_lteq(x->left_end_x, TILE_PX_LENGTH);
	twi_assert_lt(x->num_middle_tiles, SCREEN_TILE_WIDTH);
	twi_assert_lteq(x->right_end_x, TILE_PX_LENGTH);
	twi_assert_lt(tile_y_offset, TILE_PX_LENGTH);
	//--- End argument validation ---
	
	const uint32_t* original_dest = dest;
	const uint_fast8_t y_byte_offset = tile_y_offset * 2; // 2 bytes per line

	//--------------------------------------------
	// Draw the leftmost tile specially, which is
	// necessary if the entire tile isn't drawn.
	//--------------------------------------------
	const uint8_t* bg_gfx = fetch_bg_gfx(*(line_codes + line_codes_offset++), gfx);
	dest += draw_bg_tile_line(dest, palette, bg_gfx + y_byte_offset, x->left_begin_x, x->left_end_x);
	line_codes_offset %= BG_TILE_LENGTH; // wrap
	
	//------------------------------------------------------
	// Draw tiles between the left and rightmost tiles.
	// These tiles will always have their full width drawn.
	//------------------------------------------------------
	uint_fast8_t num_tiles = x->num_middle_tiles;
	while (num_tiles-- > 0) {
		bg_gfx = fetch_bg_gfx(*(line_codes + line_codes_offset++), gfx);
		dest += draw_bg_tile_line(dest, palette, bg_gfx + y_byte_offset, 0, TILE_PX_LENGTH);
		line_codes_offset %= BG_TILE_LENGTH; // wrap
	} // end main line draw loop
	
	//----------------------------------------------
	// Draw the rightmost tile specially, which is
	// necessary if the entire tile isn't drawn.
	//----------------------------------------------
	if (env->right_end_x != 0) {
		bg_gfx = fetch_bg_gfx(*(line_codes + line_codes_offset), gfx);
		draw_bg_tile_line(dest, palette, bg_gfx + y_byte_offset, 0, x->right_end_x);
	}

	return dest - original_dest; // Number of pixels drawn
} // end draw_bg_line_dmg()

//=======================================================================
// def draw_bg_tile_line()
// TODO
//=======================================================================
static inline uint_fast8_t
draw_bg_tile_line(
		uint32_t* dest,
		const uint32_t palette[4],
		const uint8_t bg_tile[2],
		uint_fast8_t x_begin,
		uint_fast8_t x_end
) {
	//--- Begin argument validation ---
	// NOTE: Extremely hot code.
	// Comment out tests if they prove too impactful on performance.
	twi_assert_notnull(dest);
	twi_assert_notnull(palette);
	twi_assert_notnull(bg_tile);
	twi_assert_lteqi(x_end, TILE_PX_LENGTH);
	twi_assert_lt(x_begin, x_end);
	//--- End argument validation ---
	
	// TODO: This description should be in the file's description.
	// Color palette consists of 4 colors, chosen by 2 bits
	// Bits are paired via equivalent offset into respective bytes,
	// and determine pixels left-to-right in order of most signficiant to least.
	// bg_char[1] contains high bits, bg_char[0] contains low bits
	// So, for example, the lowest order bits of the two-byte pair
	// determine the tile's rightmost pixel's color.
	uint_fast8_t pos = (TILE_PX_LENGTH-1) - x_begin;
	uint_fast8_t end = TILE_PX_LENGTH - x_end;
	for (; pos >= end; --pos, ++dest) {
		*dest = palette[(((bg_tile[1] >> pos) & 0x1) << 1) | ((bg_tile[0] >> pos) & 0x1)];
	} // end iteration through pixels of tile line
	return (x_end - x_begin);
} // end draw_bg_tile_line()

//=======================================================================
// decl draw_tile()
// def draw_tile()
// TODO
//=======================================================================
static inline void
draw_tile(
		uint32_t* dest,
		uint32_t palette[4],
		uint8_t gfx[16],
		const struct twi_gb_ppu_tile* tile
) {
	twi_assert_notnull(tile);
	twi_assert_gteqi(tile->y_begin, 0);
	twi_assert_lt(tile->y_begin, tile->y_end);
	twi_assert_lt(tile->y_end, TILE_PX_LENGTH);

	for (uint_fast8_t y = y_begin; y < y_end; ++y, dest += SCREEN_PX_WIDTH)
		draw_tile_line(dest, palette, gfx + (y*2), tile->x_shift_begin, tile->x_shift_end);
} // end draw_tile()

//=======================================================================
// decl draw_tile_line()
// def draw_tile_line()
// TODO
//=======================================================================
static inline void
draw_tile_line(
		uint32_t* dest,
		const uint32_t palette[4],
		const uint8_t gfx[2],
		int_fast8_t shift_begin,
		int_fast8_t shift_end
) {
	twi_assert_notnull(dest);
	twi_assert_notnull(palette);
	twi_assert_notnull(gfx);
	twi_assert_lt(shift_begin, UINT8_WIDTH);
	twi_assert_gt(shift_begin, shift_end);
	twi_assert_lteqi(shift_end, -1);

	uint_fast16_t high = gfx[1] << 1;
	for (int_fast8_t shift = shift_begin; shift > shift_end; --shift)
		*(dest++) = palette[((high >> shift) & 0x2) | ((gfx[0] >> shift) & 0x1)];
} // end draw_tile_line()

//=======================================================================
// decl resolve_pixel_color()
// def resolve_pixel_color()
// TODO
//=======================================================================
static inline uint32_t
resolve_pixel_color(
		uint32_t palette[4],
		uint8_t gfx[2],
		uint_fast8_t shift
) {
	twi_assert_notnull(palette);
	twi_assert_notnull(gfx);
	twi_assert_lti(shift, UINT8_WIDTH);
	return palette[(((gfx[1] >> shift) & 0x1) >> 1) | ((gfx[0] >> shift) & 0x1)];
} // end resolve_pixel_color()

//=======================================================================
// def fetch_bg_gfx()
// TODO
//=======================================================================
static inline const uint8_t*
fetch_bg_gfx(
		uint8_t code,
		const uint8_t* gfx[2]
) {
	twi_assert_notnull(gfx[0]);
	twi_assert_notnull(gfx[1]);

	// Positions of tile data banks 0 and 1 are not necessary subsequent,
	// hence the need for a separate pointer to each.
	// `line` is multiplied by 2 because each line is represented by two bytes.
	return ((code < BG_TILE_DATA_SIZE) ? gfx[0] : (gfx[1] - BG_TILE_DATA_SIZE));
} // end fetch_bg_gfx()

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
	twi_assert_notnull(mem_ctl);

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

