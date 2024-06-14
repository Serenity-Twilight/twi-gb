#include <assert.h>
#include <stdio.h>
#include "gb/core.h"
#include "gb/log.h"
#include "gb/mem/io.h"
#include "gb/mode.h"
#include "gb/ppu.h"
#include "gb/video/sdl.h"

enum hex_glyph_test_constants {
	NUM_HEX_DIGITS = 0x10,
	HEX_DIGIT_WIDTH = 3,
	HEX_DIGIT_HEIGHT = 5,

	PAD_SZ = 1, // Number of pixels of padding

	HEXTEST_FRAME_WIDTH = HEX_DIGIT_WIDTH + PAD_SZ * 2, // padding on each side
	HEXTEST_OUTPUT_WIDTH = HEXTEST_FRAME_WIDTH * NUM_HEX_DIGITS,

	HEXTEST_FRAME_HEIGHT = HEX_DIGIT_HEIGHT + PAD_SZ * 2, // padding on each side
	HEXTEST_OUTPUT_HEIGHT = HEXTEST_FRAME_HEIGHT,
	
	HEXTEST_OUTPUT_SIZE = HEXTEST_OUTPUT_WIDTH * HEXTEST_OUTPUT_HEIGHT
}; // end enum hex_glyph_test_constants

enum test_bg_hex_constants {
	TILE_SIDE_LENGTH = 8, // Number of pixels per any side of a tile
	VRAM_TILE_LINE_SIZE = 2, // Number of bytes for a line of 1 tile in VRAM
	// Number of bytes for 1 tile in VRAM:
	VRAM_TILE_SIZE = VRAM_TILE_LINE_SIZE * TILE_SIDE_LENGTH,
	
	// The number of bytes for drawing 1 hex digit:
	VRAM_HEX_SIZE = HEX_DIGIT_HEIGHT * VRAM_TILE_LINE_SIZE,
}; // test_bg_hex_constants 

// 15-bits, each representing 1 pixel of a monochrome 3x5 
// hexidecimal digit glyph.
//
// Layout:
// [14][13][12]
// [11][10][09]
// [08][07][06]
// [05][04][03]
// [02][01][00]
//
// High bits are colored, low bits are blank
// Bit [15] is unused.
static const uint16_t hex_glyphs[NUM_HEX_DIGITS] = {
	0x7B6F, // 0
	0x2C97, // 1
	0x62A7, // 2
	0x72CF, // 3
	0x5BC9, // 4
	0x798E, // 5
	0x79EF, // 6
	0x7252, // 7
	0x7BEF, // 8
	0x7BC9, // 9
	0x7BED, // A
	0x6BAE, // B
	0x7927, // C
	0x6B6E, // D
	0x79A7, // E
	0x79A4, // F
}; // end hex_glyphs

static void
test_hex_glyphs();
static inline void
draw_monochar(uint32_t* restrict dst, uint16_t char_bits,
		size_t x, size_t y, size_t line_width);
static void
test_bg_hex();
static void
vram_encode_X8(uint8_t* restrict vram_dst, uint8_t value);
static inline void
vram_encode_hex_glyph(uint8_t* restrict vram_dst,
		uint16_t glyph, uint8_t xoff);

int
main() {
	SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	//test_hex_glyphs();
	test_bg_hex();
	SDL_Quit();
	return 0;
} // end main()

static void
test_bg_hex() {
	struct gb_ppu ppu;
	if (gb_ppu_init(&ppu))
		return;

	ppu.state.mode = GBMODE_DMG;
	ppu.state.lcdc =
		IO_LCDC_PPU_ENABLED |
		IO_LCDC_BG_ENABLED |
		IO_LCDC_BG_TILEDATA;
	ppu.state.scy = ppu.state.scx = 0;
	ppu.state.palette = (uint8_t*)0xE4E4E4;
	uint_fast16_t tile_data_size = 0x1000;
	uint8_t value = 0x00;
	for (uint_fast16_t i = 0; i < tile_data_size; i += VRAM_TILE_SIZE)
		vram_encode_X8(ppu.state.vram + i, value++);
	value = 0x00;
	for (uint_fast16_t i = 0x1800; i < 0x1C00; ++i)
		ppu.state.vram[i] = value++;
//	value = 0x00;
//	for (uint_fast16_t i = 0x1C00; i < 0x2000; ++i)
//		ppu.state.vram[i] = value++;
	gb_dmg_draw(&ppu);
	fgetc(stdin);
} // end test_bg_hex()

static void
vram_encode_X8(uint8_t* restrict vram_dst, uint8_t value) {
	// White out tile's background:
	for (uint_fast8_t b = 0; b < VRAM_TILE_SIZE; ++b)
		vram_dst[b] = 0;

	// Draw hex digits to tile:
	vram_dst += VRAM_TILE_LINE_SIZE * 2; // Pad top 2 lines
	uint16_t mshex = hex_glyphs[(value & 0xF0) >> 4];
	uint16_t lshex = hex_glyphs[value & 0x0F];
	vram_encode_hex_glyph(vram_dst, mshex, 1); // Higher digit
	vram_encode_hex_glyph(vram_dst, lshex, 5); // Lower digit
} // end encode_hex_glyph_in_gb_vram

static inline void
vram_encode_hex_glyph(uint8_t* restrict vram_dst,
		uint16_t glyph, uint8_t xoff) {
	assert(vram_dst != NULL);
	assert(xoff <= TILE_SIDE_LENGTH - HEX_DIGIT_WIDTH); // Don't draw out-of-bounds

	uint_fast16_t hex_bit = 0x4000; // start at bit 14
	uint_fast8_t vram_bit_init = 0x80 >> xoff;
	for (uint_fast8_t y = 0; y < VRAM_HEX_SIZE; y += 2) {
		uint_fast8_t vram_bit = vram_bit_init;
		uint_fast8_t vram_bit_end = vram_bit >> HEX_DIGIT_WIDTH;
		while (vram_bit > vram_bit_end) {
			if (glyph & hex_bit) { // draw black
				vram_dst[y  ] |= vram_bit; // low bit
				vram_dst[y+1] |= vram_bit; // high bit
			}
			vram_bit >>= 1;
			hex_bit >>= 1;
		} // end drawing glyph line to vram
	} // end drawing glyph to vram
} // end vram_encode_hex_glyph

static void
test_hex_glyphs() {
//	LOGD("HEXTEST_OUTPUT_WIDTH=%d, HEXTEST_OUTPUT_HEIGHT=%d",
//			HEXTEST_OUTPUT_WIDTH, HEXTEST_OUTPUT_HEIGHT);
	struct gb_video_sdl vid;
	struct gb_video_sdl_params params = {
		.window_title = "Hex test",
		.window = { .width = HEXTEST_OUTPUT_WIDTH * 10, .height = HEXTEST_OUTPUT_HEIGHT * 10 },
		.output = { .width = HEXTEST_OUTPUT_WIDTH, .height = HEXTEST_OUTPUT_HEIGHT }
	};
	if (gb_video_sdl_init(&vid, &params))
		return;

	uint32_t* dst = gb_video_sdl_start_drawing(&vid, NULL);
	if (dst == NULL)
		goto vid_destroy;

	// --- Color background white.
	for (size_t px = 0; px < HEXTEST_OUTPUT_SIZE; ++px)
		dst[px] = 0xFFFFFFFF;

	// Draw hex digits
	size_t x = PAD_SZ;
	for (uint_fast8_t h = 0; h < NUM_HEX_DIGITS; ++h) {
		draw_monochar(dst, hex_glyphs[h], x, PAD_SZ, HEXTEST_OUTPUT_WIDTH);
		x += HEXTEST_FRAME_WIDTH;
	}
		
	if (gb_video_sdl_finish_drawing(&vid))
		goto vid_destroy;
	fgetc(stdin);
vid_destroy:
	gb_video_sdl_destroy(&vid);
} // end test_hex_glyphs()

static inline void
draw_monochar(uint32_t* restrict dst, uint16_t char_bits,
		size_t x, size_t y, size_t line_width) {
	dst += y * line_width + x;
	for (uint_fast16_t bit = 0x4000; bit;) {
		uint_fast16_t bit_end = bit >> 3;
		for (; bit > bit_end; bit >>= 1)
			*(dst++) = (char_bits & bit ? 0xFF000000 : 0xFFFFFFFF);
		dst += (line_width - 3);
	}
} // end draw_monochar()

