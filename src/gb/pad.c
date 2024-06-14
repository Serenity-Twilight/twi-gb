#include <stdint.h>
#include "gb/mem/io.h"
#include "gb/pad.h"

enum {
	GBPAD_DPAD_INPUTS =
		GBPAD_RIGHT |
		GBPAD_LEFT |
		GBPAD_UP |
		GBPAD_DOWN,
	GBPAD_BUTTON_INPUTS =
		GBPAD_A |
		GBPAD_B |
		GBPAD_SELECT |
		GBPAD_START,

	JOYP_INPUT_BITS = 0x0F,
	JOYP_DPAD_SHIFT = 0,
	JOYP_BUTTON_SHIFT = 4,
};

uint8_t
gb_pad_init() {
	return 0xFF; // All inputs released
} // end gb_pad_init()

uint8_t
gb_pad_press(uint8_t pad, uint8_t inputs) {
	// Pressed inputs are low (0)
	// This mimics the behavior of the Game Boy's JOYP register.
	return pad & ~inputs;
} // end gb_pad_press()

uint8_t
gb_pad_release(uint8_t pad, uint8_t inputs) {
	// Released inputs are high (1)
	// This mimics the bahavior of the Game Boy's JOYP register.
	return pad | inputs;
} // end gb_pad_release()

uint8_t
gb_pad_joyp(uint8_t pad, uint8_t mask) {
	// Pressed buttons and enabled masks are low (0) in JOYP.
	// Therefore, if no masks are active, then all bits of
	// JOYP are high (1).
	uint8_t joyp = 0xFF;
	if (mask & IO_JOYP_READ_DPAD) {
		// Clear DPAD selection bit
		// Clear bits for pressed DPAD inputs.
		//
		// Logical ORing the complement of JOYP_INPUT_BITS (~0x0F == 0xF0)
		// ensures that the unused inputs are ignored when
		// logically ANDing onto the mask bit complement.
		joyp &= (~IO_JOYP_READ_DPAD & ((pad >> JOYP_DPAD_SHIFT) | ~JOYP_INPUT_BITS));
	}
	if (mask & IO_JOYP_READ_BUTTONS) {
		// Same as above, but for button inputs.
		joyp &= (~IO_JOYP_READ_BUTTONS & ((pad >> JOYP_BUTTON_SHIFT) | ~JOYP_INPUT_BITS));
	}

	return joyp;
} // end gb_pad_joyp()

