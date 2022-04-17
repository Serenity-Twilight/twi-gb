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
// TODO
//-----------------------------------------------------------------------
//=======================================================================
#ifndef TWI_GB_LCD_H
#define TWI_GB_LCD_H

#include <stdint.h>
#include <twi/gb/mem.h>
#include <twi/gb/sdl.h>

//=======================================================================
//-----------------------------------------------------------------------
// External type definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// decl struct twi_gb_lcd
// def struct twi_gb_lcd
// TODO
//=======================================================================
struct twi_gb_lcd {
	// Pointer to vid so that it can be initialized/owned by something else.
	struct twi_gb_sdlvid* backend;
	uint32_t dmg_colors[4];
};

//=======================================================================
//-----------------------------------------------------------------------
// External function declarations
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// decl twi_gb_lcd_draw()
// TODO
//=======================================================================
uint_fast8_t
twi_gb_lcd_draw(
		struct twi_gb_lcd* lcd,
		const struct twi_gb_mem* mem
);

#endif // TWI_GB_LCD_H

