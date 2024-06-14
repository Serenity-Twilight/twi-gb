//=======================================================================
//-----------------------------------------------------------------------
// gb/sch.h
// TODO
//-----------------------------------------------------------------------
//=======================================================================
#ifndef GB_SCH_H
#define GB_SCH_H
#include <assert.h>
#include <stdio.h>
#include "gb/core/decl.h"

//=======================================================================
//-----------------------------------------------------------------------
// External constant definitions
//-----------------------------------------------------------------------
//=======================================================================
enum gb_schev_id {
	SCHEV_HEADER = 0,
	SCHEV_PPU,
	SCHEV_DIV,
	SCHEV_TIMA,
	SCHEV_SERIAL,
	SCHEV_TIMELIMIT,

	NUM_SCHEVS,
	SCHEV_DISABLED = 0xFFFE,
	SCHEV_NONE = 0xFFFF
}; // end enum gb_schev_id

// Every time a scheduled event fires, it must be rescheduled.
// The number of cycles before it fires again varies per event, and some
// events (such as PPU mode changes) vary based on the current state of
// the device.
//
// Rescheduling should be done by the event handler.

// Retifying the following error will require:
// 1. Increasing the bit width of member `next` in `struct gb_sch`
// 2. Increasing the bit width of member `next` in `struct gb_schev`
// 3. Increasing `SCHEV_DISABLED` to a higher value
// 4. Increasing `SCHEV_NONE` to a higher value
static_assert(NUM_SCHEVS <= SCHEV_DISABLED,
		"Too many events enumerate using uint16_t.\n");

//=======================================================================
//-----------------------------------------------------------------------
// External type definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// doc struct gb_schev
// A scheduler event.
//
// The identity and consequent behavior of the event is determined by
// its index within the array of events in `struct gb_sch`. No such
// information exists within objects of this type.
//-----------------------------------------------------------------------
// Members:
// * next:
//   The index of the event scheduled to fire after this event.
// * until:
//   The number of cycles before this event fires AFTER all events
//   scheduled to fire before this one fire.
//   In other words, the number of cycles before this event fires is
//   equal to the sum of the value of `until` in this event and the sum
//   of the values of `until` in all events that will fire before this one.
//=======================================================================
// def struct gb_schev
struct gb_schev {
	int16_t until;
	uint16_t next;
}; // end struct gb_schev

//=======================================================================
// doc struct gb_sch
// Scheduler for the emulated Game Boy.
// Tracks when events will fire, measured in emulated CPU cycles.
//-----------------------------------------------------------------------
// Members:
// * next:
//   The index of the event scheduled to fire next.
// * ev:
//   A static array of scheduler events.
//   Each distinct event's position within the array is absolute,
//   defined by its index specified in `enum gb_schev_id`.
//=======================================================================
// def struct gb_sch
struct gb_sch {
	struct gb_schev ev[NUM_SCHEVS];
}; // end struct gb_sch

//=======================================================================
//-----------------------------------------------------------------------
// External function declarations
//-----------------------------------------------------------------------
//=======================================================================
void
gb_sch_init(struct gb_core* restrict core);
void
gb_sch_advance(struct gb_core* restrict core, uint8_t cycles);
void
gb_sch_on_div_reset(struct gb_core* restrict core);
void
gb_sch_on_tac_update(
		struct gb_core* restrict core,
		uint8_t old_tac, uint8_t new_tac);
void
gb_sch_on_lcdc_update(
		struct gb_core* restrict core,
		uint8_t old_lcdc, uint8_t new_lcdc);

#endif // GB_SCHED_H

