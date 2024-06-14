#ifndef GB_PPU_SHARED_H
#define GB_PPU_SHARED_H
#include <stdint.h>

enum {
	// Screen dimensions
	PPU_SCR_WIDTH = 160, // pixels
	PPU_SCR_HEIGHT = 144, // pixels
	
	// Tile data constants
	PPU_TILE_LENGTH = 8, // pixels, length of any side
	PPU_TILE_LEN = PPU_TILE_LENGTH, // pixels, exists for compatibility
	PPU_TILE_ROW_SIZE = 2, // bytes
	PPU_TILE_SIZE = PPU_TILE_ROW_SIZE * PPU_TILE_LENGTH, // bytes

	// Background dimensions
	PPU_BG_LENGTH = 256, // pixels, for any side
	PPU_BG_TILES_PER_LENGTH = PPU_BG_LENGTH / PPU_TILE_LENGTH,

	// Size of a single object's entry in the OAM
	PPU_OAM_ENTRY_SIZE = 4, // bytes
	// Byte offsets for a single entry:
	PPU_OAM_YPOS = 0, // [0] Y-position
	PPU_OAM_XPOS = 1, // [1] X-position
	PPU_OAM_TILE = 2, // [2] Tile data index
	PPU_OAM_ATTR = 3, // [3] Attribute flags
	
	// Attribute bits
	PPU_ATTR_CGBPAL     = 0x07, // [0-2]: CGB palette (CGB only)
	PPU_ATTR_BANK       = 0x08, // [3]: VRAM bank of tile data (CGB only)
	PPU_ATTR_DMGPAL     = 0x10, // [4]: DMG palette (DMG OAM only)
	PPU_ATTR_XFLIP      = 0x20, // [5]: 0 = Normal, 1 = Mirror horizontally
	PPU_ATTR_YFLIP      = 0x40, // [6]: 0 = Normal, 1 = Mirror vertically
	PPU_ATTR_BGPRIORITY = 0x80, // [7]: 0 = Normal, 1 = Draw BG colors 1-3 over OBJ colors

	PPU_ATTR_OAMDMG =
		PPU_ATTR_BGPRIORITY | PPU_ATTR_YFLIP | PPU_ATTR_XFLIP | PPU_ATTR_DMGPAL,

	// Object constants
	PPU_OBJ_YOFF = 16, // pixels, from the top edge of the screen
	PPU_OBJ_XOFF = 8, // pixels, from the left edge of the screen
	PPU_MAX_OBJS_PER_LINE = 10,
	PPU_MAX_LINE_OAM_SIZE = PPU_OAM_ENTRY_SIZE * PPU_MAX_OBJS_PER_LINE,

	// Window constants
	PPU_WND_YOFF = 0, // pixels, from the top edge of the screen
	PPU_WND_XOFF = 7, // pixels, from the left edge of the screen
	
	// Encoded color bits
	PPU_ENC_PALETTE_TYPE = 0x80, // [7]: 0 = OBJ palette, 1 = BG palette
	PPU_ENC_PRIORITY     = 0x40, // [6]: 0 = Any color can be overwritten
															 //      1 = Colors 1-3 cannot be overwritten
	PPU_ENC_PALETTE      = 0x1C, // [4-2]: Palette (0-7)
	PPU_ENC_COLOR        = 0x03, // [1-0]: Color (0-3)
	// Palette type constants
	PPU_ENC_PALETTE_OBJ = 0x00, // bit[7] clear
	PPU_ENC_PALETTE_BG  = 0x80, // bit[7] set
	// Encoded data sizes
	PPU_ENC_PALETTE_SZ   = 3, // bits
	PPU_ENC_COLOR_SZ     = 2, // bits
	// Convenience macros for reading encoded colors
#define PPU_ENC_IS_BG(pixel_data) ((pixel_data) & PPU_ENC_PALETTE_TYPE)
#define PPU_ENC_IS_OBJ(pixel_data) (!((pixel_data) & PPU_ENC_PALETTE_TYPE))
#define PPU_ENC_HAS_PRIORITY(pixel_data) ((pixel_data) & PPU_ENC_PRIORITY)
#define PPU_ENC_GET_PALETTE(pixel_data) (((pixel_data) & PPU_ENC_PALETTE) >> 2)
#define PPU_ENC_GET_COLOR(pixel_data) ((pixel_data) & PPU_ENC_COLOR)
	
	// Number of palettes per type
	// Type = BG or OBJ
	PPU_NUM_CGB_PALETTES = 8,
};

void
gb_ppu_draw_line(
		uint32_t dst[PPU_SCR_WIDTH],
		const struct gb_ppu* restrict ppu,
		uint8_t line);

#endif // GB_PPU_SHARED_H

