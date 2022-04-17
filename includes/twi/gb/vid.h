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
// TODO
//=======================================================================
#ifndef TWI_GB_VID_H
#define TWI_GB_VID_H

#include <stdint.h>
#include <SDL2/SDL_video.h>
#include <twi/gb/mem.h>

//=======================================================================
//-----------------------------------------------------------------------
// Public type definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// decl struct twi_gb_vid
// def struct twi_gb_vid
// TODO
//=======================================================================
struct twi_gb_vid {
	SDL_Window* window;
	SDL_GLContext gl;
	uint32_t fbo;
	uint32_t vram_ubo;
	uint32_t oamctl_ubo;
	uint16_t winx0;
	uint16_t winx1;
	uint16_t winy0;
	uint16_t winy1;
	uint8_t vram_seg;
	uint8_t oamctl_seg;
	uint8_t vram_num_segs;
	uint8_t oamctl_num_segs;
	uint8_t flags;
}; // end struct twi_gb_vid

//=======================================================================
//-----------------------------------------------------------------------
// Public function declarations
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// decl twi_gb_vid_init()
// TODO
//=======================================================================
uint_fast8_t
twi_gb_vid_init(struct twi_gb_vid* vid);

//=======================================================================
// decl twi_gb_vid_destroy()
// TODO
//=======================================================================
void
twi_gb_vid_destroy(struct twi_gb_vid* vid);

//=======================================================================
// decl twi_gb_vid_draw()
// TODO
//=======================================================================
void
twi_gb_vid_draw(struct twi_gb_vid* vid, const struct twi_gb_mem* mem);

//=======================================================================
// decl twi_gb_vid_onchange_resolution()
// TODO
//=======================================================================
void
twi_gb_vid_onchange_resolution(struct twi_gb_vid* vid);

#endif // TWI_GB_VID_H

