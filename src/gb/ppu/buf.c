#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include "gb/mem/region.h"
#include "gb/mode.h"
#include "gb/ppu/state.h"

enum {
	PPU_MAX_BUFFERS = 32,
};

struct gb_ppu_buf {
	uint8_t* statebuf;
	uint8_t* vrambuf;
	uint8_t* oambuf;
	uint8_t* palettebuf;
	uint32_t statebuf_usage;
	uint32_t vrambuf_usage;
	uint32_t oambuf_usage;
	uint32_t palettebuf_usage;
	uint8_t statebuf_curr;
	uint8_t vrambuf_curr;
	uint8_t oambuf_curr;
	uint8_t palettebuf_curr;
	uint8_t mode;
}; // end struct gb_ppu_buf
#define BUF_USAGE_BITCOUNT(buffer_name) \
	((sizeof (((struct gb_ppu_buf*)0)->buffer_name##_usage)) * CHAR_BIT)
static_assert(BUF_USAGE_BITCOUNT(statebuf) >= PPU_MAX_BUFFERS);
static_assert(BUF_USAGE_BITCOUNT(vrambuf) >= PPU_MAX_BUFFERS);
static_assert(BUF_USAGE_BITCOUNT(oambuf) >= PPU_MAX_BUFFERS);
static_assert(BUF_USAGE_BITCOUNT(palettebuf) >= PPU_MAX_BUFFERS);

struct gb_ppu_buf*
gb_ppu_buf_create(enum gb_mode mode) {
	// Calculate cumulative size of buffers which represent PPU state.
	size_t oam_bufsz = MEM_OAM_SZ * PPU_MAX_BUFFERS;
	size_t state_bufsz, vram_bufsz, palette_bufsz;
	if (mode != GBMODE_CGB) {
		state_bufsz =
			(sizeof(struct gb_ppu_state) + PPU_DMGPAL_SZ) * PPU_MAX_BUFFERS;
		vram_bufsz = MEM_VRAM_SZ * PPU_MAX_BUFFERS;
		palette_bufsz = 0; // DMG palettes stored in state object
	} else {
		state_bufsz = sizeof(struct gb_ppu_state) * PPU_MAX_BUFFERS;
		vram_bufsz = MEM_VRAM_SZ * 2 * PPU_MAX_BUFFERS;
		palette_bufsz = PPU_CGBPAL_SZ * PPU_MAX_BUFFERS;
	}

	size_t bufsz = state_bufsz + vram_bufsz + oam_bufsz + palette_bufsz;
	struct gb_ppu_buf* buf = malloc(sizeof(struct gb_ppu_buf) + bufsz);
	if (buf == NULL)
		return NULL;

	// Initialize buffer pointers.
	void* offset = buf;
	buf->statebuf = (offset += sizeof(struct gb_ppu_buf));
	buf->vrambuf = (offset += state_bufsz);
	buf->oambuf = (offset += vram_bufsz);
	buf->palettebuf = (offset += oam_bufsz);
	assert((offset + palette_bufsz) - (buf + sizeof(struct gb_ppu_buf)) == bufsz);

	// Clear all bits of buffer usage trackers.
	buf->statebuf_usage = buf->vrambuf_usage =
		buf->oambuf_usage = buf->palettebuf_usage = 0;
	// Initialize current buffer to last buffer, so first usage wraps to first buffer.
	buf->statebuf_curr = buf->vrambuf_curr =
		buf->oambuf_curr = buf->palettebuf_curr = PPU_MAX_BUFFERS - 1;
	buf->mode = mode;

	return buf;
} // end gb_ppu_buf_create()

