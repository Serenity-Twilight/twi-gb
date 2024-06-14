#ifndef GB_PAD_H
#define GB_PAD_H
#include <stdint.h>
#include "gb/mem/io.h"

enum gb_pad_input {
	// D-pad
	GBPAD_RIGHT  = IO_JOYP_A_RIGHT,
	GBPAD_LEFT   = IO_JOYP_B_LEFT,
	GBPAD_UP     = IO_JOYP_SELECT_UP,
	GBPAD_DOWN   = IO_JOYP_START_DOWN,
	// Buttons
	GBPAD_A      = IO_JOYP_A_RIGHT << 4,
	GBPAD_B      = IO_JOYP_B_LEFT << 4,
	GBPAD_SELECT = IO_JOYP_SELECT_UP << 4,
	GBPAD_START  = IO_JOYP_START_DOWN << 4,
}; // end enum gb_pad_input

uint8_t
gb_pad_init();

uint8_t
gb_pad_press(uint8_t pad, uint8_t inputs);

uint8_t
gb_pad_release(uint8_t pad, uint8_t inputs);

uint8_t
gb_pad_joyp(uint8_t pad, uint8_t mask);

#endif // GB_PAD_H

