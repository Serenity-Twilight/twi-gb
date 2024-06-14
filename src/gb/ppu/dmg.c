#include <assert.h>
#include <stdint.h>
#include "gb/mem/io.h"
#include "gb/ppu/shared.h"

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL FUNCTION DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
//=======================================================================
// def dmg_draw_obj_line()
static void
dmg_encode_obj_row(
		uint8_t dst_line[PPU_SCR_WIDTH],
		const struct gb_ppu_state* restrict state,
		uint_fast8_t line_num) {
	assert(dst_line != NULL);
	assert(state != NULL);
	assert(line_num < PPU_SCR_HEIGHT);

	if (!(state->lcdc & IO_LCDC_OBJ_ENABLED))
		return; // Object drawing is disabled.

	uint8_t obj_indices[PPU_OBJS_PER_LINE];
	uint_fast8_t obj_count = gb_ppu_select_line_objs(
			obj_indices, state->oam, state->lcdc & IO_LCDC_OBJ_SIZE, line_num);

	for (uint_fast8_t i = 0; i < obj_count; ++i) {
		// Shared logic treats attributes as if its CGB-valid.
		// To accomodate this, following composes a new attributes byte by:
		// 1. Preserving the values of the BGPRIORITY, YFLIP, and XFLIP bits
		//    from the real attributes byte.
		// 2. Leaving the BANK bit cleared, which tells the shared logic to
		//    index tile data from VRAM bank 0, which is the only VRAM bank
		//    on DMG.
		// 3. Setting the CGBPAL bits to whatever the DMG palette is (0 or 1).
		uint_fast8_t attribs =
			(state->oam[obj_indices[i] + PPU_OAM_ATTR] &
				(PPU_ATTR_BGPRIORITY | PPU_ATTR_YFLIP | PPU_ATTR_XFLIP)) |
			((state->oam[obj_indices[i] + PPU_OAM_ATTR] & PPU_OAM_ATTR_DMGPAL) != 0);
		gb_ppu_encode_obj_tile_row(dst, state, obj_indices[i], attribs, line_num, 
	}
} // end dmg_encode_obj_row()

static inline uint_fast8_t
dmg_encode_obj_row_tile(
		uint8_t dst_line[PPU_SCR_WIDTH],
		const struct gb_ppu_state* restrict state,
		uint_fast8_t obj_index,
		uint_fast8_t prev_obj_xend,
		uint_fast8_t line_num) {
	assert(dst_line != NULL);
	assert(state != NULL);
	assert(obj_index < SZ_OAM);
	assert(obj_index % PPU_OAM_ENTRY_SIZE == 0);
	assert(line_num < PPU_SCR_HEIGHT);


} // end dmg_encode_obj_row_tile()

