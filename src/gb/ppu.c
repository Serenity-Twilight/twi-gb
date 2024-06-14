#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <SDL.h>
#include "gb/core.h"
#include "gb/log.h"
#include "gb/mem/io.h"
#include "gb/ppu.h"
#include "gb/ppu/shared.h"
#include "gb/video/sdl.h"

//=======================================================================
//-----------------------------------------------------------------------
// Internal constant definitions
//-----------------------------------------------------------------------
//=======================================================================
enum {
	// Screen height and width, and total output display pixel count.
	NUM_PIXELS = PPU_SCR_HEIGHT * PPU_SCR_WIDTH,
	PIXEL_SIZE = 4, // in bytes
	OUTPUT_SIZE = NUM_PIXELS * PIXEL_SIZE,

	// Number of tiles along each axis.
	TILES_PER_AXIS = 256 / PPU_TILE_LEN, // 32 = 256 / 8
	
	// VRAM region offsets
	VRAM_DATA0 = 0x0000,
	VRAM_DATA1 = 0x0800,
	VRAM_DATA2 = 0x1000,
	VRAM_BGMAP0 = 0x1800,
	VRAM_BGMAP1 = 0x1C00,

	// Number of bytes in a tile's color index, both per row of
	// the tile and for the entirety of the tile.
	TILE_DATA_LINE_SIZE = 2,
	TILE_DATA_SIZE = TILE_DATA_LINE_SIZE * PPU_TILE_LEN
};

enum palette {
	BGP,  // DMG background palette
	OBP0, // DMG object palette 0
	OBP1, // DMG object palette 1
	NUM_DMG_PALETTES
}; // end enum palette

// Built-in DMG LCD colors
enum greyscale_palette {
	GREYSCALE_WHITE = 0xFFFFFFFF,
	GREYSCALE_LGREY = 0xFFAAAAAA,
	GREYSCALE_DGREY = 0xFF555555,
	GREYSCALE_BLACK = 0xFF000000
};

//=======================================================================
//-----------------------------------------------------------------------
// Internal type definitions
//-----------------------------------------------------------------------
//=======================================================================
struct tile_bounds {
	int_fast16_t pos;
	uint_fast8_t begin;
	uint_fast8_t end;
}; // end struct tile

//=======================================================================
//-----------------------------------------------------------------------
// Internal function declarations
//-----------------------------------------------------------------------
//=======================================================================
//static void
//dmg_draw_bg(
//		uint32_t pixels[NUM_PIXELS],
//		const struct gb_ppu* restrict ppu);
//static void
//draw_tile(
//		uint32_t pixels[NUM_PIXELS],
//		const uint32_t palette[4],
//		const uint8_t color_index[TILE_DATA_SIZE],
//		int_fast16_t ypos, int_fast16_t xpos);
//static inline uint_fast8_t
//clamp_tile_bounds(struct tile_bounds* bounds, uint_fast8_t max_pos);
//static inline uint8_t
//color_code(const uint8_t tile_colormap[TILE_DATA_LINE_SIZE], uint8_t bit);

//=======================================================================
//-----------------------------------------------------------------------
// External function definitions
//-----------------------------------------------------------------------
//=======================================================================
// def gb_ppu_init()
int
gb_ppu_init(struct gb_ppu* restrict ppu) {
	struct gb_video_sdl_params params = {
		.window_title="Game Boy",
		.window = { .width = PPU_SCR_WIDTH * 3, .height = PPU_SCR_HEIGHT * 3 },
		.output = { .width = PPU_SCR_WIDTH, .height = PPU_SCR_HEIGHT }
	};
	if (gb_video_sdl_init(&(ppu->target), &params))
		return 1;

	void* mem = malloc(MEM_SZ_VRAM + MEM_SZ_OAM);
	if (mem == NULL)
		goto destroy_video;
	ppu->state.vram = mem;
	ppu->state.oam = mem + MEM_SZ_VRAM;
	ppu->dmg_colors[0] = GREYSCALE_WHITE;
	ppu->dmg_colors[1] = GREYSCALE_LGREY;
	ppu->dmg_colors[2] = GREYSCALE_DGREY;
	ppu->dmg_colors[3] = GREYSCALE_BLACK;
	
	return 0;
destroy_video:
	gb_video_sdl_destroy(&(ppu->target));
	return 1;
} // end gb_ppu_init()

//=======================================================================
// def gb_ppu_destroy()
void
gb_ppu_destroy(struct gb_ppu* restrict ppu) {
	free(ppu->state.vram);
	gb_video_sdl_destroy(&(ppu->target));
} // end gb_ppu_destroy()

//=======================================================================
// def gb_dmg_draw()
uint8_t
gb_dmg_draw(struct gb_ppu* restrict ppu) {
	if (!(ppu->state.lcdc & IO_LCDC_PPU_ENABLED)) {
		//fputs("PPU IS OFF!\n", stderr);
		return gb_video_sdl_draw_clear(&(ppu->target));
	}

	uint32_t* pixels = gb_video_sdl_start_drawing(&(ppu->target), NULL);
	if (pixels == NULL)
		return 1;

	static const uint8_t LINE_LIMIT = PPU_SCR_HEIGHT;
	uint8_t i = 0;
	for (; i < LINE_LIMIT; ++i) {
		//LOGD("pixels = %p", pixels);
		gb_ppu_draw_line(pixels, ppu, i);
		pixels += PPU_SCR_WIDTH;
	}

	if (gb_video_sdl_finish_drawing(&(ppu->target)))
		return 1;
	return 0;
} // end gb_dmg_draw()

//=======================================================================
//-----------------------------------------------------------------------
// Internal function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// doc dmg_draw_bg()
// TODO
//=======================================================================
// def dmg_draw_bg()
//static void
//dmg_draw_bg(
//		uint32_t pixels[NUM_PIXELS],
//		const struct gb_ppu* restrict ppu) {
//	uint32_t bg_palette[4] = {
//		ppu->palette[(ppu->state.bgp     ) & 0x3],
//		ppu->palette[(ppu->state.bgp >> 2) & 0x3],
//		ppu->palette[(ppu->state.bgp >> 4) & 0x3],
//		ppu->palette[(ppu->state.bgp >> 6) & 0x3]
//	};
//	LOGT("bg_palette = { %08" PRIX32 ", %08" PRIX32 ", %08" PRIX32 ", %08" PRIX32 " }",
//			bg_palette[0], bg_palette[1], bg_palette[2], bg_palette[3]);
//
//	LOGD("scy=%" PRIu8 ", scx=%" PRIu8, ppu->state.scy, ppu->state.scx);
//	// The starting address of the BG tile map in use.
//	uint_fast16_t tile_map = (ppu->state.lcdc & IO_LCDC_BG_TILEMAP ? VRAM_BGMAP1 : VRAM_BGMAP0);
//	LOGD("tile_map=0x%04" PRIX16 ", lcdc.3=%" PRIu8, 
//			(uint16_t)(tile_map),
//			(ppu->state.lcdc & IO_LCDC_BG_TILEMAP) != 0);
//	// Value that gets XOR'd against the background tile data index.
//	// This is needed because if IO_LCDC_BG_TILEDATA == 0, then
//	// tile data indices 0-127 come immediately AFTER indices 128-255.
//	// So the index will need its 7th bit flipped in that case.
//	uint8_t tile_data_index_xor;
//	// The starting address of the BG tile data.
//	uint_fast16_t tile_data;
//	if (ppu->state.lcdc & IO_LCDC_BG_TILEDATA) {
//		//   0-127: 0x8000-0x87FF
//		// 128-255: 0x8800-0x8FFF
//		tile_data_index_xor = 0; // No bit flip necessary.
//		tile_data = VRAM_DATA0;
//	} else {
//		//   0-127: 0x9000-0x97FF
//		// 128-255: 0x8800-0x8FFF
//		tile_data_index_xor = 0x80; // Flip bit 7
//		tile_data = VRAM_DATA1;
//	}
//	LOGD("tile_data_index_xor=0x%02" PRIX8 ", tile_data=0x%04" PRIX16
//			", lcdc.4=%" PRIu8,
//			tile_data_index_xor, (uint16_t)(tile_data),
//			(ppu->state.lcdc & IO_LCDC_BG_TILEDATA) != 0);
//
//	//--- Iterate through all partially or fully onscreen tiles ---
//	// Render target y-coordinate, kind of.
//	// These coordinates begin at the top-leftmost pixel of the
//	// top-leftmost ONscreen tile, even if that top-leftmost pixel
//	// is OFFscreen. The tile drawing function culls and scrolls
//	// out-of-bounds tile coordinates automatically.
//	int_fast16_t y = -(ppu->state.scy % PPU_TILE_LEN);
//	// Y-offset of the next tile map entry.
//	uint8_t tile_map_y = ppu->state.scy / PPU_TILE_LEN;
//	while (y < PPU_SCR_HEIGHT) {
//		// Render target x-coordinate, kind of (see comment for `y`).
//		int_fast16_t x = -(ppu->state.scx % PPU_TILE_LEN);
//		// X-offset of the next tile map entry.
//		uint8_t tile_map_x = ppu->state.scx / PPU_TILE_LEN;
//		while (x < PPU_SCR_WIDTH) {
//			// Look up index
//			uint_fast16_t index_ptr =
//				tile_map + (tile_map_y * TILES_PER_AXIS) + tile_map_x;
//			uint8_t index = (ppu->state.vram[index_ptr] ^ tile_data_index_xor);
//			uint_fast16_t data_ptr = tile_data + index * TILE_DATA_SIZE;
//			// Look up data.
//			LOGD("\n\ty=%" PRId16 ", x=%" PRId16
//					"\n\ttile_map_y=%" PRIu8 ", tile_map_x=%" PRIu8
//					"\n\tindex_ptr=0x%04" PRIX16
//					"\n\tindex=0x%02" PRIX8 " (xor=0x%02" PRIX8 ")",
//					y, x, tile_map_y, tile_map_x, index_ptr,
//					index, tile_data_index_xor);
//			draw_tile(pixels, bg_palette, ppu->state.vram + data_ptr, y, x);
//
//			x += PPU_TILE_LEN;
//			tile_map_x = (tile_map_x + 1) % TILES_PER_AXIS; // wrap
//		} // end x-iteration
//		y += PPU_TILE_LEN;
//		tile_map_y = (tile_map_y + 1) % TILES_PER_AXIS; // wrap
//	} // end y-iteration
//} // end dmg_draw_bg()
//
////=======================================================================
//// doc draw_tile()
//// TODO
////=======================================================================
//// def draw_tile()
//static void
//draw_tile(
//		uint32_t pixels[NUM_PIXELS],
//		const uint32_t palette[4],
//		const uint8_t tile_data[TILE_DATA_SIZE],
//		int_fast16_t ypos, int_fast16_t xpos) {
//	//------------------------------------
//	// Cull tile pixels that fall outside of the bounds of the screen.
//	struct tile_bounds ybounds = {.pos=ypos};
//	if (!clamp_tile_bounds(&ybounds, PPU_SCR_HEIGHT))
//		return; // Tile outside of y-bounds, will not be drawn.
//	struct tile_bounds xbounds = {.pos=xpos};
//	if (!clamp_tile_bounds(&xbounds, PPU_SCR_WIDTH))
//		return; // Tile outside of x-bounds, will not be drawn.
//
//	//------------------------------------
//	// Draw tile to the designated region, ignoring culled edges.
//	pixels += (ybounds.pos * PPU_SCR_WIDTH) + xbounds.pos;
//	const uint8_t* tile_data_end = tile_data + ybounds.end * TILE_DATA_LINE_SIZE;
//	tile_data += ybounds.begin * TILE_DATA_LINE_SIZE;
//	while (tile_data < tile_data_end) {
//		uint32_t* pixels_row_start = pixels;
//		for (uint_fast8_t bit = xbounds.begin; bit < xbounds.end; ++bit)
//			*(pixels++) = palette[color_code(tile_data, 7 - bit)];
//		pixels = pixels_row_start + PPU_SCR_WIDTH;
//		tile_data += TILE_DATA_LINE_SIZE;
//	}
//} // end draw_tile()
//
//static inline uint_fast8_t
//clamp_tile_bounds(struct tile_bounds* bounds, uint_fast8_t max_pos) {
//	if (bounds->pos < -PPU_TILE_LEN || bounds->pos > max_pos)
//		return 0; // Tile is completely out-of-bounds, and will not be drawn.
//	if (bounds->pos < 0) {
//		// Tile overflows out of lower bounds.
//		// Begin after out-of-bounds pixels.
//		bounds->begin = -(bounds->pos);
//		bounds->end = PPU_TILE_LEN;
//		// Move starting pixel for drawing in bounds.
//		bounds->pos = 0;
//	} else if (bounds->pos + (PPU_TILE_LEN-1) > max_pos) {
//		// Tile overflows out of upper bounds.
//		// End before out-of-bounds pixels.
//		bounds->begin = 0;
//		bounds->end = max_pos - (bounds->pos-1);
//	} else {
//		bounds->begin = 0;
//		bounds->end = PPU_TILE_LEN;
//	}
//
//	return 1;
//} // end clamp_tile_bounds()
//
//static inline uint8_t
//color_code(const uint8_t tile_data[TILE_DATA_LINE_SIZE], uint8_t bit) {
//	assert(bit < 8);
//	return (((tile_data[1] >> bit) & 0x1) << 1) // high bit
//	      | ((tile_data[0] >> bit) & 0x1);      // low bit
//} // end color_code()
//
