//======================================================================
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
// TODO: Description
//-----------------------------------------------------------------------
//=======================================================================
#ifndef TWI_GB_SDL_H
#define TWI_GB_SDL_H

#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>

//=======================================================================
// decl struct twi_gb_sdl
// def struct twi_gb_sdl
//
// TODO
//=======================================================================
struct twi_gb_sdlvid {
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;
	uint32_t* pixels;
};

//=======================================================================
// decl twi_gb_sdlvid_init()
//
// TODO
//=======================================================================
uint_fast8_t
twi_gb_sdlvid_init(struct twi_gb_sdlvid* vid);

//=======================================================================
// decl twi_gb_sdlvid_destroy()
//
// TODO
//=======================================================================
void
twi_gb_sdlvid_destroy(struct twi_gb_sdlvid* vid);

//=======================================================================
// decl twi_gb_sdlvid_get_drawing_area()
//
// TODO
//=======================================================================
uint32_t*
twi_gb_sdlvid_get_drawing_area(struct twi_gb_sdlvid* vid);

//=======================================================================
// decl twi_gb_sdlvid_draw()
//
// TODO
//=======================================================================
uint_fast8_t
twi_gb_sdlvid_draw(struct twi_gb_sdlvid* vid);

//=======================================================================
// decl twi_gb_sdl_draw()
// TODO
//=======================================================================
//void
//twi_gb_sdl_draw(struct twi_gb_sdl* sdl);

//=======================================================================
// decl twi_gb_sdl_draw()
// TODO
//=======================================================================
//void
//twi_gb_sdl_draw_test(struct twi_gb_sdl* sdl);

#endif // TWI_GB_SDL_H

