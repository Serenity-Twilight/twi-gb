#include <inttypes.h>
#include <stdint.h>
#include "gb/core.h"
#include "gb/cpu.h"
#include "gb/log.h"
#include "gb/mem.h"
#include "gb/mem/io.h"
#include "gb/pad.h"

#define GB_LOG_MAX_LEVEL LVL_DBG

//=======================================================================
//-----------------------------------------------------------------------
// Internal function declarations
//-----------------------------------------------------------------------
//=======================================================================
static inline void
lycompare(struct gb_core* restrict core);

//=======================================================================
//-----------------------------------------------------------------------
// External function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def gb_mem_io_request_interrupt()
void
gb_mem_io_request_interrupt(
		struct gb_core* restrict core,
		enum io_ife_bit interrupt) {
	core->mem.map[IO_IF] |= interrupt;
	if (core->mem.ime && (core->mem.map[IO_IE] & interrupt))
		gb_cpu_interrupt(core, 1);
} // end gb_mem_io_request_interrupt()

//=======================================================================
// def gb_mem_io_clear_interrupt()
void
gb_mem_io_clear_interrupt(
		struct gb_core* restrict core,
		enum io_ife_bit interrupt) {
	core->mem.map[IO_IF] &= ~interrupt;
	if (core->mem.ime)
		gb_cpu_interrupt(core, gb_mem_io_pending_interrupts(core));
} // end gb_mem_io_clear_interrupt()

//=======================================================================
// def gb_mem_io_on_ifie_write()
void
gb_mem_io_on_ifie_write(struct gb_core* restrict core) {
	if (core->mem.ime)
		gb_cpu_interrupt(core, gb_mem_io_pending_interrupts(core));
} // end gb_mem_io_on_ifie_write()

//=======================================================================
// def gb_mem_io_pending_interrupts()
uint8_t
gb_mem_io_pending_interrupts(struct gb_core* restrict core) {
	return core->mem.map[IO_IF] & core->mem.map[IO_IE] & ~IO_IFE_UNUSED;
} // end gb_mem_io_pending_interrupts();

//=======================================================================
// def gb_mem_io_set_ime()
void
gb_mem_io_set_ime(struct gb_core* restrict core, uint8_t enable) {
	LOGT("core=%p, enable=%" PRIu8, core, enable);
	core->mem.ime = enable;
	uint8_t interrupt = enable && gb_mem_io_pending_interrupts(core);
	gb_cpu_interrupt(core, interrupt);
} // end gb_mem_io_set_ime()

//=======================================================================
// def gb_mem_io_get_ime()
uint8_t
gb_mem_io_get_ime(const struct gb_core* restrict core) {
	return core->mem.ime;
} // end gb_mem_io_get_ime()

//=======================================================================
// def gb_mem_io_set_lyc()
void
gb_mem_io_set_lyc(struct gb_core* restrict core, uint8_t new_lyc) {
	uint8_t old_stat_int = core->mem.stat_int;
	core->mem.map[IO_LYC] = new_lyc;
	lycompare(core);
	if (!old_stat_int && core->mem.stat_int)
		gb_mem_io_request_interrupt(core, IO_IFE_STAT);
} // end gb_mem_io_set_lyc()

//=======================================================================
// def gb_mem_io_advance_ppu()
uint8_t
gb_mem_io_advance_ppu(struct gb_core* restrict core) {
	// Identify current state, advance to next state,
	// trigger any STAT interrupts as appropriate.
	uint8_t stat = core->mem.map[IO_STAT];
	uint8_t old_stat_int = core->mem.stat_int;
	switch (stat & IO_STAT_MODE) {
		case 0: // Horizontal blank
			core->mem.map[IO_LY] += 1;
			core->mem.stat_int &= ~(IO_STAT_INT_MODE0 | IO_STAT_INT_LYC);
			lycompare(core);

			if (core->mem.map[IO_LY] == 144) { // begin vertical blank
				core->mem.map[IO_STAT] += 1; // Mode 0->1
				core->mem.stat_int |= (stat & IO_STAT_INT_MODE1); // Check mode 1 stat interrupt
				gb_mem_io_request_interrupt(core, IO_IFE_VBLANK);
			} else { // begin OAM scan
				core->mem.map[IO_STAT] += 2; // Mode 0->2
				core->mem.stat_int |= (stat & IO_STAT_INT_MODE2); // Check mode 2 stat interrupt
			}
			break;
		case 1: // Vertical blank
			core->mem.map[IO_LY] += 1;
			core->mem.stat_int &= ~IO_STAT_INT_LYC;
			if (core->mem.map[IO_LY] == 154) { // begin OAM scan
				core->mem.map[IO_LY] = 0; // Wrap line counter
				core->mem.stat_int &= ~IO_STAT_INT_MODE1;
				core->mem.map[IO_STAT] += 1; // Mode 1->2
				core->mem.stat_int |= (stat & IO_STAT_INT_MODE2);
			}
			lycompare(core);
			break;
		case 2: // OAM scan
			// Begin pixel-drawing
			core->mem.stat_int &= ~IO_STAT_INT_MODE2;
			core->mem.map[IO_STAT] += 1; // Mode 2->3
			break;
		case 3: // Pixel-drawing
			// Begin horizontal blank
			core->mem.map[IO_STAT] -= 3; // Mode 3->0
			core->mem.stat_int |= (stat & IO_STAT_INT_MODE0);
			break;
	} // end switch (mode)
	
	// If stat interrupt line was low before (== 0) and went high,
	// then request an interrupt.
	if (!old_stat_int && core->mem.stat_int)
		gb_mem_io_request_interrupt(core, IO_IFE_STAT);
	return core->mem.map[IO_STAT] & IO_STAT_MODE;
} // end gb_mem_io_advance_ppu()

//=======================================================================
// def gb_mem_io_increment_div()
void
gb_mem_io_increment_div(struct gb_core* restrict core) {
	core->mem.map[IO_DIV] += 1;
} // end gb_mem_io_increment_div()

//=======================================================================
// def gb_mem_io_increment_tima()
void
gb_mem_io_increment_tima(struct gb_core* restrict core) {
	core->mem.map[IO_TIMA] += 1;
	if (core->mem.map[IO_TIMA] == 0) { // Overflow
		core->mem.map[IO_TIMA] = core->mem.map[IO_TMA];
		gb_mem_io_request_interrupt(core, IO_IFE_TIMER);
	}
} // end gb_mem_io_increment_tima()

//=======================================================================
// def gb_mem_io_update_joyp()
void
gb_mem_io_update_joyp(struct gb_core* restrict core, uint8_t gb_pad) {
	uint8_t old_joyp = core->mem.map[IO_JOYP];
	uint8_t new_joyp = core->mem.map[IO_JOYP] =
		gb_pad_joyp(gb_pad, ~old_joyp & IO_JOYP_MASKS);

	// The following expression detects high-to-low changes in input bits.
	// 1. XOR detects changes
	// 2. AND `~new_joyp` filters out low-to-high changes
	// Examples:
	// 0->0 = (0 ^ 0) & 1 = 0
	// 0->1 = (0 ^ 1) & 0 = 0
	// 1->0 = (1 ^ 0) & 1 = 1
	// 1->1 = (1 ^ 1) & 0 = 0
	uint8_t high_to_low =
		((old_joyp ^ new_joyp) & ~new_joyp) & IO_JOYP_INPUTS;
	LOGD("joyp= 0x%02X -> 0x%02X, high-to-low=0x%X",
			old_joyp, new_joyp, high_to_low);
	if (high_to_low)
		gb_mem_io_request_interrupt(core, IO_IFE_JOYP);
} // end gb_mem_io_update_joyp()

//=======================================================================
//-----------------------------------------------------------------------
// Internal function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// doc lycompare()
// TODO
//=======================================================================
// def lycompare()
static inline void
lycompare(struct gb_core* restrict core) {
	if (core->mem.map[IO_LY] == core->mem.map[IO_LYC]) {
		// Set STAT LYMATCH bit, set LYC bit in stat interrupt line if enabled.
		core->mem.map[IO_STAT] |= IO_STAT_LYMATCH;
		core->mem.stat_int |= (core->mem.map[IO_STAT] & IO_STAT_INT_LYC);
	} else {
		// Clear STAT LYMATCH bit, clear LYC bit in stat interrupt line.
		core->mem.map[IO_STAT] &= ~IO_STAT_LYMATCH;
		core->mem.stat_int &= ~IO_STAT_INT_LYC;
	}
} // end lycompare()

