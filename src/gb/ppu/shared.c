#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include "gb/core/decl.h"
#include "gb/log.h"
#include "gb/mem/io.h"
#include "gb/mem/region.h"
#include "gb/mode.h"
#include "gb/ppu.h"
#include "gb/ppu/shared.h"

#define GB_LOG_MAX_LEVEL LVL_INF

//=======================================================================
// The following is an outline of what information is needed for each
// stage of drawing, and what information can be shared between stages.
//
// Note that frames are drawn one line at a time, to accomodate the
// Game Boy's unconventional allowance for modifying frame appearance
// data mid-frame.
//-----------------------------------------------------------------------
// Drawing a line:
// 	1. BG, until WND starts
// 	2. WND
// 	3. OBJ, respecting BG/WND priorities
//-----------------------------------------------------------------------
// Shared per-line info:
// 	1. PPU enabled (LCDC.7)
// 	2. BG enabled (DMG) -or- BG priority yield (CGB) (LCDC.0)
//     Needed by BG/WND on DMG to fill line with white pixels
//     Needed by OBJs to ignore BG priority of tiles
//
// Shared BG/WND per-line info:
// 	1. WND enabled (LCDC.5)
// 	2. BG/WND tile data pointer, random access (LCDC.4)
// 	3. BG/WND tile data index XOR (LCDC.4)
//  5. Starting X-offset, inclusive
//     (WND: wx - 7, else 0)
//  6. Ending X-offset, exclusive
//     (BG when WND is enabled: wx - 7, else 160)
//
// BG per-line info:
// 	1. BG tilemap, accessed sequentially with line wrap (LCDC.3)
// 	2. Y-offset into tile ((line + scy) % tile_length)
// 	3. Beginning tilemap offset (scx % tile_length)
//
// WND per-line info:
// 	1. WND tilemap, accessed sequentially, no wrap (LCDC.6)
// 	2. Y-offset into tile ((line - wy) % tile_length)
//
// OBJ per-line info:
// 	1. OBJs enabled (LCDC.1)
// 	2. OBJ size (LCDC.2) (can be integrated into y-offset)
//-----------------------------------------------------------------------
// Shared per-tile info:
// 	1. Palette (DMG: attr.4, CGB: attr.0-2)
//  2. VRAM Bank (attr.3, applies VRAM-sized offset to tile data pointer)
//  3. Xflip (attr.5, reverses tile data traversal direction)
//  4. Yflip (attr.6, inverts tile data line offset
//      (bytes_per_tile - (line * bytes_per_line)))
//  5. BG priority (attr.7, ignored if LCDC.0 is clear, any model)
//  6. screen X-coordinate of tile's left edge
//     ([249 (-7),159], used for culling offscreen pixels)
//
// OBJ per-tile info:
//  1. screen Y-coordinate of tile's top edge
//     ([0,143], used for determing Y-offset into tile)
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL CONSTANT DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================
enum {
	OBJ_SMALL_HEIGHT = 8, // pixels, when lcdc.2 == 0
	OBJ_LARGE_HEIGHT = OBJ_SMALL_HEIGHT * 2, // 16 pixels, when lcdc.2 == 1
	
	DMG_NUM_PALETTES = 3,
	DMG_PALETTE_BITS = 8,
	DMG_COLOR_BITS = 2,
	COLORS_PER_PALETTE = 4,

	DMG_NUM_COLORS = DMG_NUM_PALETTES * COLORS_PER_PALETTE,
	// On CGB:
	// 4 colors per palette
	// 8 palettes per layer
	// 2 layers (OBJ,BG)
	// 4 * 8 * 2 = 64
	CGB_NUM_COLORS = 64,

	OBP0_OFFSET = 0,
	OBP1_OFFSET = COLORS_PER_PALETTE,
	BGP_OFFSET = COLORS_PER_PALETTE * 2,
	
	// VRAM tile data bank offsets
	// NOTE: Offset values originate from the start of
	// the VRAM memory region (0x8000-0x9FFF).
	//
	// Object tile data indices map to:
	// 0-127 -> VRAM_DATA0 (0x0000-0x07FF)
	// 128-255 -> VRAM_DATA1 (0x0800-0x0FFF)
	// BG/Window tile data indices map to:
	// if lcdc.4 == 0: 0-127 -> VRAM_DATA2 (0x1000-0x17FF)
	//                 128-255 -> VRAM_DATA1 (0x0800-0x0FFF)
	// if lcdc.4 == 1: 0-127 -> VRAM_DATA0 (0x0000-0x07FF)
	//
	// NOTE: When lcdc.4 == 0, 0-127 is mapped AFTER 128-255
	VRAM_DATA0 = 0x0000,
	VRAM_DATA1 = 0x0800,
	VRAM_DATA2 = 0x1000,
	VRAM_BGMAP0 = 0x1800,
	VRAM_BGMAP1 = 0x1C00,
};

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL TYPE DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================

struct bg_shared_info {
	uint8_t* tiledata;
	uint8_t tile_index_xor;
	uint8_t gb_mode;
}; // end struct bg_shared_info

struct bg_layer_info {
	const uint8_t* tilemap;
	uint8_t bg_row;
	uint8_t bg_col_init;
	uint8_t width;
}; // end struct bg_layer_info

struct obj_info {
	uint8_t bg_yields_priority;
	uint8_t line;
	uint8_t double_height;
	uint8_t is_cgb;
};

struct tile_info {
	uint8_t index;
	uint8_t attribs;
	uint8_t row;
	uint8_t end_x;
	int16_t x;
}; // end struct tile_info

struct shift_bits {
	int8_t curr; // Current bit
	int8_t end;  // Ending bit
	int8_t incr; // Incrementation direction and scale
}; // end struct shift_bits

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL VARIABLE DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================
thread_local const uint8_t* sorting_oam;

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL FUNCTION DECLARATIONS
//-----------------------------------------------------------------------
//=======================================================================
static void
gb_ppu_encode_bg_row(
		uint8_t dst[PPU_SCR_WIDTH],
		const struct gb_ppu_state* restrict state,
		uint8_t screen_row);
static uint8_t*
gb_ppu_encode_bg_layer_row(
		uint8_t* restrict dst,
		const struct bg_shared_info* restrict shared,
		const struct bg_layer_info* restrict layer);
static uint8_t*
gb_ppu_encode_bg_tile_row(
		uint8_t* restrict dst,
		const uint8_t* restrict tiledata,
		const struct tile_info* restrict info);
static void
encode_obj_row(
		uint8_t* restrict dst,
		const struct gb_ppu_state* restrict state,
		uint8_t line);
static void
encode_obj_tile_row(
		uint8_t* restrict dst,
		const uint8_t* restrict vram,
		const struct obj_info* restrict obj,
		const struct tile_info* restrict tile);
static inline uint8_t
select_line_objs(
		uint8_t obj_indices[PPU_MAX_OBJS_PER_LINE],
		const uint8_t* restrict oam,
		const struct obj_info* restrict info);
static inline void
bit_shift_parameters(
		struct shift_bits* restrict shift,
		const struct tile_info* restrict tile);
static inline uint_fast8_t
color_code(const uint8_t data[PPU_TILE_ROW_SIZE], uint_fast8_t bit);
static inline struct bg_shared_info
create_bg_shared_info(const struct gb_ppu_state* restrict state);
static inline const uint8_t*
get_bg_tilemap_ptr(
		const struct gb_ppu_state* restrict state,
		uint8_t tilemap_bitmask);
static inline uint16_t
get_tile_data_row_index(
		const struct tile_info* restrict tile,
		uint8_t double_size);
static inline void
resolve_palettes(
		uint32_t* restrict colors,
		const uint8_t* restrict gb_palettes,
		const uint32_t dmg_colors[4],
		uint8_t gb_mode);
static int
x_sort_objs(const void* obj1, const void* obj2);

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL FUNCTION DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================

//#undef GB_LOG_MAX_LEVEL
//#define GB_LOG_MAX_LEVEL LVL_TRC
void
gb_ppu_draw_line(
		uint32_t dst[PPU_SCR_WIDTH],
		const struct gb_ppu* restrict ppu,
		uint8_t line) {
	uint8_t enc[PPU_SCR_WIDTH];
	gb_ppu_encode_bg_row(enc, &(ppu->state), line);
	encode_obj_row(enc, &(ppu->state), line);

	uint32_t colors[CGB_NUM_COLORS];
	resolve_palettes(colors, ppu->state.palette, ppu->dmg_colors, ppu->state.mode);
	if (ppu->state.mode != GBMODE_CGB) {
		// DMG
		for (uint8_t i = 0; i < PPU_SCR_WIDTH; ++i) {
			if (PPU_ENC_IS_OBJ(enc[i])) // OBJ
				dst[i] = colors[enc[i] & (PPU_ENC_PALETTE | PPU_ENC_COLOR)];
			else // BG
				dst[i] = colors[(enc[i] & PPU_ENC_COLOR) + BGP_OFFSET];
		}
	} else {
		assert(0); // to-be-implemented
	}
} // end gb_ppu_draw_line()
//#undef GB_LOG_MAX_LEVEL
//#define GB_LOG_MAX_LEVEL LVL_INF

//=======================================================================
static void
gb_ppu_encode_bg_row(
		uint8_t dst[PPU_SCR_WIDTH],
		const struct gb_ppu_state* restrict state,
		uint8_t screen_row) {
	assert(state != NULL);
	assert(screen_row < PPU_SCR_HEIGHT);

	if (state->mode != GBMODE_CGB &&
	    !(state->lcdc & IO_LCDC_BG_ENABLED)) {
		// (DMG-only) BG is disabled
		// Fill dst with deprioritized color 0 pixels.
		for (uint8_t i = 0; i < PPU_SCR_WIDTH; ++i)
			dst[i] = PPU_ENC_PALETTE_BG;
	}

	struct bg_shared_info shared = create_bg_shared_info(state);
	// Subtract offset to get screen x-coordinate of window:
	int16_t wnd_x = (int16_t)(state->wx) - PPU_WND_XOFF; 
	uint8_t wnd_visible = // window visibility
		(state->lcdc & IO_LCDC_WND_ENABLED && // window is enabled
		 state->wy <= screen_row &&           // window (y) has begun by this line
		 wnd_x < PPU_SCR_WIDTH);              // window (x) begins before line ends
	uint8_t bg_visible = (!wnd_visible || wnd_x > 0);

	//--- Background layer ---
	if (bg_visible) {
		struct bg_layer_info layer = {
			.tilemap = get_bg_tilemap_ptr(state, IO_LCDC_BG_TILEMAP),
			.bg_row = screen_row + state->scy, // Overflow is intentional.
			.bg_col_init = state->scx,
			.width = (wnd_visible ? wnd_x : PPU_SCR_WIDTH)
		}; // end layer definition
		dst = gb_ppu_encode_bg_layer_row(dst, &shared, &layer);
	} // end if (bg_visible)
	
	//--- Window layer ---
	if (wnd_visible) {
		// Window is visible
		struct bg_layer_info layer = {
			.tilemap = get_bg_tilemap_ptr(state, IO_LCDC_WND_TILEMAP),
			.bg_row = screen_row - state->wy,
			.bg_col_init = 0,
			.width = PPU_SCR_WIDTH - wnd_x
		}; // end layer definition
		gb_ppu_encode_bg_layer_row(dst, &shared, &layer);
	} // end if (wnd_visible)
	// What's fundamentally different between BG and WND?
	// 1. Tilemap region determined by different LCDC bits.
	// 2. On DMG:
	//    BG_ENABLED == 0: BOTH off
	//    BG_ENABLED == 1: BG on
	//    On CGB: (no difference between BG and WND on this)
	//    BG always drawn, BG_ENABLED sets both's priority
	// 3. WND_ENABLED == 0: WND off, BG can be on
	// 4. Starting and ending columns
	//    (passing as width)
} // end gb_ppu_encode_bg_row()

//=======================================================================
// def gb_ppu_encode_bg_layer_row()
static uint8_t*
gb_ppu_encode_bg_layer_row(
		uint8_t* restrict dst,
		const struct bg_shared_info* restrict shared,
		const struct bg_layer_info* restrict layer) {
	assert(shared != NULL);
	assert(layer != NULL);
	uint8_t tilemap_row = layer->bg_row / PPU_TILE_LENGTH;
	uint16_t row_ptr = tilemap_row * PPU_BG_TILES_PER_LENGTH;
	LOGT("tilemap_row=%u, row_ptr=0x%04X", tilemap_row, row_ptr);
	uint8_t tilemap_col = layer->bg_col_init / PPU_TILE_LENGTH;

	struct tile_info tile;
	tile.row = layer->bg_row % PPU_TILE_LENGTH;
	// X-coordinate of tile's leftmost column of pixels:
	// TODO: This is wrong for window layer
	tile.x = -((int16_t)(layer->bg_col_init % PPU_TILE_LENGTH));
	tile.end_x = layer->width;

	LOGT("shared->tile_index_xor=0x%02X", shared->tile_index_xor);
	LOGT("tile.row = %" PRIu8 ", tile.end_x = %" PRIu8, tile.row, tile.end_x);
	while (tile.x < tile.end_x) {
		uint16_t tile_ptr = row_ptr + tilemap_col;
		tile.index = layer->tilemap[tile_ptr] ^ shared->tile_index_xor;
		tile.attribs = (shared->gb_mode == GBMODE_CGB) ?
			layer->tilemap[tile_ptr + MEM_SZ_VRAM] : 0;

		LOGT("dst = %p, tile.x = %" PRId16 ", tile.index = %" PRIu8 ", tilemap_col = %" PRIu8,
				dst, tile.x, tile.index, tilemap_col);
		dst = gb_ppu_encode_bg_tile_row(dst, shared->tiledata, &tile);
		tilemap_col = (tilemap_col + 1) % PPU_BG_TILES_PER_LENGTH;
		tile.x += PPU_TILE_LENGTH;
	} // end while (tile.x < tile.end_x)
	return dst;
} // end gb_ppu_encode_bg_layer_row()

//=======================================================================
// def gb_ppu_encode_bg_tile_row()
//#undef GB_LOG_MAX_LEVEL
//#define GB_LOG_MAX_LEVEL LVL_TRC
static uint8_t*
gb_ppu_encode_bg_tile_row(
		uint8_t* restrict dst,
		const uint8_t* restrict tiledata,
		const struct tile_info* restrict info) {
	assert(dst != NULL);
	assert(tiledata != NULL);
	assert(info != NULL);
	
	// Pointer to tile color data in VRAM:
	uint16_t data_index = get_tile_data_row_index(info, 0);
	// Calculate encoded palette, minus color, which will be
	// applied later when looping over this object's pixels.
	// NOTE: DMG callers are expected to zero all attributes.
	uint8_t enc_palette = 
		PPU_ENC_PALETTE_BG |
		(info->attribs & PPU_ATTR_BGPRIORITY ? PPU_ENC_PRIORITY : 0) |
		(info->attribs & PPU_ATTR_CGBPAL) << PPU_ENC_COLOR_SZ;
	LOGD("enc_palette=0x%02X & PPU_ENC_COLOR=0x%02X = 0x%02X",
			enc_palette, PPU_ENC_COLOR, enc_palette & PPU_ENC_COLOR);
	assert((enc_palette & PPU_ENC_COLOR) == 0);
	// Cull offscreen pixels and determine bit traversal order:
	struct shift_bits bit;
	bit_shift_parameters(&bit, info);
	LOGT("bit={.curr=%d,.end=%d,.incr=%d}", bit.curr, bit.end, bit.incr);
	// Encode color data to dst.
	while (bit.curr != bit.end) {
		*(dst++) = enc_palette | color_code(tiledata + data_index, bit.curr);
		bit.curr += bit.incr;
	}
	return dst;
} // end gb_ppu_encode_bg_tile_row()
//#undef GB_LOG_MAX_LEVEL
//#define GB_LOG_MAX_LEVEL LVL_INF

//=======================================================================
// def encode_obj_row()
static void
encode_obj_row(
		uint8_t* restrict dst,
		const struct gb_ppu_state* restrict state,
		uint8_t line) {
	if (!(state->lcdc & IO_LCDC_OBJ_ENABLED))
		return;

	struct obj_info obj_info = {
		.bg_yields_priority = !(state->lcdc & IO_LCDC_BG_ENABLED),
		.line = line,
		.double_height = state->lcdc & IO_LCDC_OBJ_SIZE,
		.is_cgb = state->mode == GBMODE_CGB
	};

	uint8_t objv[PPU_MAX_OBJS_PER_LINE];
	uint8_t objc = select_line_objs(objv, state->oam, &obj_info);
	LOGD("obj_info={.line=%u,.double_height=%u}, objc = %u",
			obj_info.line, obj_info.double_height, objc);

	if (objc == 0)
		return;

	struct tile_info tile_info;
	tile_info.end_x = PPU_SCR_WIDTH;
	for (uint8_t i = 0; i < objc; ++i) {
		uint8_t x = state->oam[objv[i] + PPU_OAM_XPOS];
		if (x == 0 || x >= PPU_SCR_WIDTH + PPU_OBJ_XOFF)
			continue; // Tile is completely offscreen.

		LOGD("objv[%u]=%u->{Y=%u,X=%u}", i, objv[i],
				state->oam[objv[i] + PPU_OAM_YPOS],
				state->oam[objv[i] + PPU_OAM_XPOS]);
		tile_info.index = state->oam[objv[i] + PPU_OAM_TILE];
		tile_info.attribs = state->oam[objv[i] + PPU_OAM_ATTR];
		// OBJ tile encoding expects a CGB-valid attribute byte.
		// On DMG, ensure the VRAM bank bit is cleared, and the CGB palette
		// bits indicate the correct DMG object palette:
		if (!(obj_info.is_cgb)) {
			tile_info.attribs &= PPU_ATTR_OAMDMG; // Clear CBGPAL and BANK bits
			// Assign DMGPAL bit to CGBPAL bits:
			tile_info.attribs |= ((tile_info.attribs & PPU_ATTR_DMGPAL) != 0);
		}
		// The tile's internal row to draw is the screen line minus the
		// object's REAL (offset subtracted) Y-coordinate.
		tile_info.row = line -
			(state->oam[objv[i] + PPU_OAM_YPOS] - PPU_OBJ_YOFF);
		tile_info.x = (int16_t)x - PPU_OBJ_XOFF;

		uint8_t dst_offset = (tile_info.x < 0 ? 0 : tile_info.x);
		encode_obj_tile_row(dst + dst_offset, state->vram, &obj_info, &tile_info);
	} // end iteration over objv
} // end encode_obj_row()

//=======================================================================
// def encode_obj_tile_row()
static void
encode_obj_tile_row(
		uint8_t* restrict dst,
		const uint8_t* restrict vram,
		const struct obj_info* restrict obj,
		const struct tile_info* restrict tile) {
	assert(dst != NULL);
	assert(obj != NULL);
	assert(tile != NULL);
	assert(tile->index < MEM_SZ_OAM);

	// Pointer to the row of tile color data in VRAM:
	uint16_t data_ptr = get_tile_data_row_index(tile, obj->double_height);

	// Calculate encoded palette, minus color, which will be
	// applied later when looping over this object's pixels.
	// Note that the priority bit is set:
	// Object pixels cannot be overwritten by other object pixels.
	// NOTE: DMG callers are expected to copy the
	//       DMG palette to the CGB palette space.
	uint8_t enc_palette =
		PPU_ENC_PALETTE_OBJ | PPU_ENC_PRIORITY |
		(tile->attribs & PPU_ATTR_CGBPAL) << PPU_ENC_COLOR_SZ;
	assert((enc_palette & PPU_ENC_COLOR) == 0);

	// Clamp usage of color bits to only onscreen pixels, and
	// determine the direction of traversal.
	struct shift_bits bit;
	bit_shift_parameters(&bit, tile);

	// Does the background globally yield priority?
	if (!obj->bg_yields_priority) {
		// Background does NOT yield priority.
		uint8_t obj_yields_priority = tile->attribs & PPU_ATTR_BGPRIORITY;
		while (bit.curr != bit.end) {
			if ((!obj_yields_priority && !PPU_ENC_HAS_PRIORITY(*dst)) ||
					!PPU_ENC_GET_COLOR(*dst)) {
				// Either:
				// 1. OBJ isn't yielding priority, AND
				//    current color isn't demanding it,
				//    -OR-
				// 2. Current color is zero, which ALWAYS yields priority.
				// OBJ colors 1-3 will take priority over BG pixels.
				uint8_t color = color_code(vram + data_ptr, bit.curr);
				if (color != 0)
					*dst = enc_palette | color;
			}
			dst += 1;
			bit.curr += bit.incr;
		} // end incrementation over color bits
	} else {
		// Background is yielding priority.
		// OBJ colors 1-3 will take priority over BG colors.
		while (bit.curr != bit.end) {
			// Any object pixels already drawn take priority, so
			// only overwrite BG pixels.
			if (PPU_ENC_IS_BG(*dst)) {
				uint8_t color = color_code(vram + data_ptr, bit.curr);
				if (color != 0)
					*dst = enc_palette | color;
			}
			dst += 1;
			bit.curr += bit.incr;
		} // end incrementation over color bits
	} // end ifelse BG does NOT yield priority
} // end gb_ppu_encode_obj_tile_row()

//=======================================================================
// def select_line_objs()
static inline uint8_t
select_line_objs(
		uint8_t obj_indices[PPU_MAX_OBJS_PER_LINE],
		const uint8_t* restrict oam,
		const struct obj_info* restrict info) {
	LOGT("begin");
	assert(oam != NULL);
	assert(info != NULL);
	assert(info->line < PPU_SCR_HEIGHT);
	// Guarantee no overflow when adding PPU_OBJ_YOFF to line:
	static_assert(PPU_SCR_HEIGHT + PPU_OBJ_YOFF < UINT8_MAX);

	//---------------------------------------------------------------------
	// The first 10 objects (ordered by position in OAM) on this line
	// are selected for drawing, regardless of X-position. Any
	// objects after the first ten are ignored, regardless of Y-position.
	//------
	// Apply object Y offset to get line relative to object coordinates.
	uint8_t offset_line = info->line + PPU_OBJ_YOFF;
	// Guarantee subtracting height from line won't overflow through 0:
	static_assert(OBJ_LARGE_HEIGHT-1 <= PPU_OBJ_YOFF);
	// Minimum Y-coordinate for visible objects on this line:
	uint8_t min_line = offset_line -
		(!(info->double_height) ? (OBJ_SMALL_HEIGHT-1) : (OBJ_LARGE_HEIGHT-1));
	// Maximum Y-coordinate for visible objects on this line:
	uint8_t max_line = offset_line;

	// Guarantee no overflow in `i` when traversing OAM:
	static_assert(MEM_SZ_OAM <= UINT8_MAX - PPU_OAM_ENTRY_SIZE);
	// Record OAM offsets for up to 10 objects on this line:
	LOGT("counting objects from Y=[%u,%u]", min_line, max_line);
	uint8_t obj_count = 0;
	for (uint8_t i = 0; i < MEM_SZ_OAM; i += PPU_OAM_ENTRY_SIZE) {
		if (oam[i + PPU_OAM_YPOS] >= min_line && oam[i + PPU_OAM_YPOS] <= max_line) {
			obj_indices[obj_count++] = i;
			if (obj_count >= PPU_MAX_OBJS_PER_LINE)
				break;
		}
	} // end iteration over OAM
	LOGT("objects on line: %u", obj_count);
	
	//---------------------------------------------------------------------
	// On DMG, objects are prioritized by order of ascending X-coordinate
	// (lower X-coordinate = higher priority),
	// with objects with matching X-coordinates prioritized by ascending
	// OAM index (lower index = higher priority).
	//
	// On CGB, object priority is determined solely by ascending OAM index
	// (lower index = higher priority).
	//
	// Priority determines how overlapping objects behave with one another.
	// Higher priority objects are drawn over lower priority objects.
	if (!(info->is_cgb) && obj_count > 1) {
		LOGD("Pre-sorting obj indices:");
		for (uint8_t i = 0; i < obj_count; ++i)
			LOGD("obj_indices[%u]=%u", i, obj_indices[i]);
		sorting_oam = oam;
		qsort(obj_indices, obj_count, sizeof(uint8_t), x_sort_objs);
		LOGD("Post-sorting obj indices:");
		for (uint8_t i = 0; i < obj_count; ++i)
			LOGD("obj_indices[%u]=%u", i, obj_indices[i]);
	}
	LOGT("end");
	return obj_count;
} // end select_line_objs()

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL FUNCTION DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
static inline void
bit_shift_parameters(
		struct shift_bits* restrict shift,
		const struct tile_info* restrict tile) {
	assert(shift != NULL);
	assert(tile != NULL);
	// Not totally off the left or right edges of the screen, respectively:
	assert(tile->x > -PPU_TILE_LENGTH);
	assert(tile->x < tile->end_x);
	assert(tile->end_x <= PPU_SCR_WIDTH);

	// Calculate the number of pixels to cull from each side,
	// if any, due to the tile being partially offscreen.
	uint8_t cull_left, cull_right;
	// Is tile partially cut off on the left edge?
	if (tile->x < 0)
		cull_left = -(tile->x);
	else
		cull_left = 0;
	// Is tile partially cut off on the right edge?
	uint8_t tile_end = tile->x + PPU_TILE_LENGTH;
	if (tile_end > tile->end_x)
		cull_right = tile_end - tile->end_x;
	else
		cull_right = 0;

	// Determine what bits to skip based on culled pixels,
	// and the direction of traversal based on the XFLIP attribute.
	// If XFLIP is set, then the graphic is flipped horizontally.
	if (!(tile->attribs & PPU_ATTR_XFLIP)) { // Not flipped
		// Traverse bits from more to less significant
		shift->curr = (PPU_TILE_LEN-1) - cull_left;
		shift->end = cull_right - 1;
		shift->incr = -1;
	} else { // Flipped horizontally
		// Traverse bits from less to more signficant
		shift->curr = cull_left;
		shift->end = PPU_TILE_LEN - cull_right;
		shift->incr = 1;
	}
} // end bit_shift_parameters()

//=======================================================================
static inline const uint8_t*
get_bg_tilemap_ptr(
		const struct gb_ppu_state* restrict state,
		uint8_t tilemap_bitmask) {
	assert(state != NULL);
	assert(tilemap_bitmask == IO_LCDC_BG_TILEMAP ||
	       tilemap_bitmask == IO_LCDC_WND_TILEMAP);
	return state->vram +
		(!(state->lcdc & tilemap_bitmask) ? VRAM_BGMAP0 : VRAM_BGMAP1);
} // end get_bg_tilemap_ptr()

//=======================================================================
static inline uint16_t
get_tile_data_row_index(
		const struct tile_info* restrict tile,
		uint8_t double_size) {
	assert(tile != NULL);
	LOGD("tile->row=%u, double_size=%u", tile->row, double_size);
	// Assert that overflow won't occur:
	static_assert((PPU_TILE_LEN * 2) - 1 <= UINT8_MAX);
	const uint8_t tile_row_max =
		(PPU_TILE_LEN * (double_size ? 2 : 1)) - 1;
	assert(tile->row <= tile_row_max);

	uint8_t row = tile->row;
	// The following inversion respects the size of the object.
	if (tile->attribs & PPU_ATTR_YFLIP) // graphic should be upside-down?
		row = tile_row_max - row; // invert row offset
	// Lowest bit of index is ignored when using double size mode:
	uint8_t index = tile->index;
	if (double_size)
		index &= ~1;

	uint16_t data_row_index =
		index * PPU_TILE_SIZE +  // offset to start of selected tile +
		row * PPU_TILE_ROW_SIZE; // offset to selected row
	if (tile->attribs & PPU_ATTR_BANK) // Data within bank 1?
		data_row_index += MEM_SZ_VRAM; // Apply size of bank 0 as an offset.
	LOGT("index=0x%02X, row=%u, data_row_index=0x%04X", index, row, data_row_index);
	return data_row_index;
} // end get_tile_data_row_index()

//=======================================================================
// def color_code()
static inline uint_fast8_t
color_code(const uint8_t data[PPU_TILE_ROW_SIZE], uint_fast8_t bit) {
	uint_fast8_t low_bit  = (data[0] >> bit) & 1;
	uint_fast8_t high_bit = (data[1] >> bit) & 1;
	return (high_bit << 1) | low_bit;
} // end color_code()

//static const uint8_t*
//dmg_obj_tile_row_data(
//		const struct gb_ppu_state* restrict state,
//		uint8_t obj_index,
//		uint8_t scr_line) {
//	uint8_t tile_index = state->oam[obj_index + PPU_OAM_TILE];
//	uint8_t tile_height;
//	if (!(state->lcdc & IO_LCDC_OBJ_SIZE)) {
//		tile_height = PPU_TILE_LEN; // Normal height
//	} else {
//		tile_height = PPU_TILE_LEN * 2; // Double height
//		tile_index &= ~1; // Least-significant bit ignored
//	}
//
//	// Calculate row WITHIN the tile to draw:
//	// OAM y-coordinate is offset by PPU_OBJ_YOFF, so this value
//	// needs to be subtracted to get the real y-coordinate,
//	// which is then compared to the screen line.
//	uint8_t tile_row =
//		scr_line - (state->oam[obj_index + PPU_OAM_YPOS] - PPU_OBJ_YOFF);
//	assert(tile_yoff < tile_height);
//	// Is the object graphic to be drawn upside-down?
//	if (state->oam[obj_index + PPU_OAM_ATTR] & PPU_ATTR_YFLIP)
//		tile_row = (tile_height-1) - tile_row; // invert row
//} // end dmg_obj_tile_row_data()

//=======================================================================
//static const uint8_t*
//cgb_obj_tile_row_data(
//		const struct gb_ppu_state* restrict state,
//		uint8_t obj_index,
//		uint8_t scr_line) {
//	const uint8_t* data =
//		dmg_obj_tile_row_data(state, obj_index, scr_line);
//	if
//} // end cgb_obj_tile_row_data()

//=======================================================================
static inline struct bg_shared_info
create_bg_shared_info(const struct gb_ppu_state* restrict state) {
	assert(state != NULL);
	// If the BG_TILEMAP bit in LCDC is...
	// 1: Indices 0-127 are mapped to VRAM_DATA0.
	// 0: indices 0-127 are mapped to VRAM_DATA2.
	// Indices 128-255 are always mapped to VRAM_DATA1.
	//
	// VRAM_DATA0 immediately precedes VRAM_DATA1,
	// and VRAM_DATA1 immediately precedes VRAM_DATA2.
	//
	// Therefore, if BG_TILEMAP == 0, then indexing begins
	// at VRAM_DATA1 and all indices have their 7th bit inverted
	// (which is the equivalent of adding 128 of an unsigned 8-bit int).
	struct bg_shared_info info;
	if (state->lcdc & IO_LCDC_BG_TILEDATA) {
		info.tiledata = state->vram + VRAM_DATA0;
		info.tile_index_xor = 0x00; // No inversion of bit 7
	} else {
		info.tiledata = state->vram + VRAM_DATA1;
		info.tile_index_xor = 0x80; // Invert bit 7
	}

	info.gb_mode = state->mode;
	LOGT("lcdc=0x%02u,info={.tiledata=%p (%p),.tile_index_xor=0x%02X,.gb_mode=%u}",
			state->lcdc, info.tiledata, info.tiledata - state->vram,
			info.tile_index_xor, info.gb_mode);
	return info;
} // end create_bg_shared_info()

//=======================================================================
// def resolve_palettes()
//#undef GB_LOG_MAX_LEVEL
//#define GB_LOG_MAX_LEVEL LVL_TRC
static inline void
resolve_palettes(
		uint32_t* restrict colors,
		const uint8_t* restrict gb_palettes,
		const uint32_t dmg_colors[4],
		uint8_t gb_mode) {
	if (gb_mode != GBMODE_CGB) {
		LOGT("DMG colors");
		// DMG has 3 palettes, each encoded into 1 byte.
		// They are stored in the lowest 3 bytes of gb_palettes
		// (meaning that gb_palettes isn't actually a pointer).
		// In order of ascending byte significance, the palettes are stored as:
		// 	OBP0, OBP1, BGP
		static_assert(DMG_NUM_PALETTES < UINT8_MAX);
		for (uint8_t p = 0; p < DMG_NUM_PALETTES; ++p) {
			// Each palette represents 4 colors.
			// Each color is represented by 2 consecutive bits.
			// Color 0 = bits[1-0]
			// Color 1 = bits[3-2]
			// Color 2 = bits[5-4]
			// Color 3 = bits[7-6]
			// Each 2-bit value indicates one of the four system colors,
			// which are stored in `dmg_colors`
			uint8_t palette =
				(((intptr_t)gb_palettes) >> (p * DMG_PALETTE_BITS)) & 0xFF;
			uint8_t c = p * COLORS_PER_PALETTE;
			colors[c  ] = dmg_colors[palette & 0x3];
			colors[c+1] = dmg_colors[(palette >> DMG_COLOR_BITS) & 0x3];
			colors[c+2] = dmg_colors[(palette >> DMG_COLOR_BITS * 2) & 0x3];
			colors[c+3] = dmg_colors[(palette >> DMG_COLOR_BITS * 3) & 0x3];

			LOGT("palette[%u]=0x%02X, colors[%u]=0x%08X,[%u]=0x%08X,[%u]=0x%08X,[%u]=0x%08X",
					p, palette, c, colors[c], c+1, colors[c+1], c+2, colors[c+2], c+3, colors[c+3]);
		}
	} else {
		assert(0); // to-be-implemented
	}
} // end resolve_palettes()
//#undef GB_LOG_MAX_LEVEL
//#define GB_LOG_MAX_LEVEL LVL_INF

//=======================================================================
// def x_sort_objs()
static int
x_sort_objs(const void* obj1, const void* obj2) {
	uint8_t obj1i = *((uint8_t*)obj1);
	uint8_t obj2i = *((uint8_t*)obj2);
	uint8_t obj1x = sorting_oam[obj1i + PPU_OAM_XPOS];
	uint8_t obj2x = sorting_oam[obj2i + PPU_OAM_XPOS];
	int diff = (int)obj1x - obj2x;
	if (diff) // not equal
		return diff;

	// Objects share an identical X-coordinate.
	// Sort by lowest index.
	return (int)obj1i - obj2i;
} // end x_sort_objs()

