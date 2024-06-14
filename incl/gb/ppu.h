#ifndef GB_PPU_H
#define GB_PPU_H
#include <stdint.h>
#include "gb/ppu/state.h"
#include "gb/video/sdl.h"

struct gb_ppu {
	struct gb_video_sdl target;
	struct gb_ppu_state state;
	uint32_t dmg_colors[4];
};

int
gb_ppu_init(struct gb_ppu* restrict ppu);
void
gb_ppu_destroy(struct gb_ppu* restrict ppu);
uint8_t
gb_dmg_draw(struct gb_ppu* restrict ppu);

#endif // GB_PPU_H

