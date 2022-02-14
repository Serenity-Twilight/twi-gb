//=======================================================================
//-----------------------------------------------------------------------
// TODO
// Written by Serenity Twilight
//-----------------------------------------------------------------------
//=======================================================================
#ifndef TWI_GB_LCD_H
#define TWI_GB_LCD_H

#include <twi/gb/mem.h>
#include <twi/gb/sdl.h>

//=======================================================================
// decl struct twi_gb_lcd
// def struct twi_gb_lcd
// TODO
//=======================================================================
struct twi_gb_lcd {
	// Pointer to vid so that it can be initialized/owned
	// by something else.
	struct twi_gb_sdlvid* vid;
};

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

