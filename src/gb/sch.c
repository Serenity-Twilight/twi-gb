#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include "gb/core/typedef.h"
#include "gb/mem.h"
#include "gb/mem/io.h"
#define GB_LOG_MAX_LEVEL LVL_TRC
#include "gb/log.h"
#include "gb/sch.h"

#define EV(index) (core->sch.ev[index])
#define EV_HEADER (EV(SCHEV_HEADER))
#define EV_FIRST (EV(EV_HEADER.next))

//=======================================================================
//-----------------------------------------------------------------------
// Internal constant definitions
//-----------------------------------------------------------------------
//=======================================================================
// Duration of each PPU mode, in cycles.
enum ppu_mode { HBLANK, VBLANK, OAM_SCAN, PIXEL_DRAW };
static const uint16_t CYC_PPU[] = { 51, 114, 20, 43 };
// Duration between incrementations of DIV, in cycles.
enum { CYC_DIV = 64 };
// Duration between incrementations of TIMA, in cycles,
// depending on the clock selected in the TAC register.
static const uint16_t CYC_TIMA[] = { 256, 4, 16, 64 };

//=======================================================================
//-----------------------------------------------------------------------
// Internal function declarations
//-----------------------------------------------------------------------
//=======================================================================
static void
execute_event(struct gb_core* restrict core);
static inline void
pause_event(struct gb_core* restrict core, enum gb_schev_id target);
static inline enum gb_schev_id
pop_event(struct gb_core* restrict core);
static inline void
remove_event(struct gb_core* restrict core, enum gb_schev_id event);
static void
insert_event(struct gb_core* restrict core, enum gb_schev_id target);
static inline uint8_t
clock_bit_state(struct gb_core* restrict core, uint8_t tac_clock_speed);

//=======================================================================
//-----------------------------------------------------------------------
// External function definitions
//-----------------------------------------------------------------------
//=======================================================================
void
gb_sch_init(struct gb_core* restrict core) {
	EV_HEADER.next = SCHEV_NONE;

	EV(SCHEV_PPU).until = CYC_PPU[OAM_SCAN];
	insert_event(core, SCHEV_PPU);
	EV(SCHEV_DIV).until = CYC_DIV;
	insert_event(core, SCHEV_DIV);

	EV(SCHEV_TIMA).next = SCHEV_DISABLED;
	EV(SCHEV_SERIAL).next = SCHEV_DISABLED;
	EV(SCHEV_TIMELIMIT).next = SCHEV_DISABLED;
} // end gb_sch_init()

void
gb_sch_advance(struct gb_core* restrict core, uint8_t cycles) {
	// There should always be at least 1 event.
	assert(EV_HEADER.next != SCHEV_NONE);

	EV_FIRST.until -= cycles;
	while (EV_FIRST.until <= 0)
		execute_event(core);
} // end gb_sch_advance()

//=======================================================================
// def gb_sch_on_div_reset()
void
gb_sch_on_div_reset(struct gb_core* restrict core) {
	remove_event(core, SCHEV_DIV);
	EV(SCHEV_DIV).until = CYC_DIV;
	insert_event(core, SCHEV_DIV);

	uint8_t tac = gb_mem_direct_read(core, IO_TAC);
	if (tac & IO_TAC_ENABLE) {
		remove_event(core, SCHEV_TIMA);
		EV(SCHEV_TIMA).until = CYC_TIMA[tac & IO_TAC_CLOCK_SELECT];
		insert_event(core, SCHEV_TIMA);
	} 
} // end gb_sch_on_div_reset()

//=======================================================================
// def gb_sch_on_tac_update()
void
gb_sch_on_tac_update(
		struct gb_core* restrict core,
		uint8_t old_tac, uint8_t new_tac) {
	if ((old_tac & IO_TAC_READWRITE) == (new_tac & IO_TAC_READWRITE))
		return; // No change in TAC
	if (!(old_tac & IO_TAC_ENABLE) && !(new_tac & IO_TAC_ENABLE))
		return; // Clock goes from off-to-off, so clock changes are irrelevant.
	
	uint8_t old_enabled = old_tac & IO_TAC_ENABLE;
	uint8_t old_clock = old_tac & IO_TAC_CLOCK_SELECT;
	uint8_t new_enabled = new_tac & IO_TAC_ENABLE;
	uint8_t new_clock = new_tac & IO_TAC_CLOCK_SELECT;
	//---------------------------------------------------------------------
	// TIMA is incremented every time a monitored bit in 16-bit DIV goes
	// from high to low. The monitored bit is chosen by TAC clock selection.
	// Due to this, changing the value of TAC whilst TIMA is running and
	// the monitored bit high will cause TIMA to increment instantaneously
	// if either:
	// 1. The timer is being disabled via TAC, or
	// 2. the timer's clock rate is changing, and the new monitored DIV
	//    bit is currently low.
	if (old_enabled) {
		if (clock_bit_state(core, old_clock)
		 && (!new_enabled || !clock_bit_state(core, new_clock)))
			gb_mem_io_increment_tima(core); // Falling edge created
	} // end if (old_enabled)
	//---------------------------------------------------------------------
	// Whether the timer is turning off or is changing clock rate, its
	// currently scheduled event is now invalid.
	remove_event(core, SCHEV_TIMA);
	if (!new_enabled)
		return; // If disabled, do not requeue.

	// Reschedule enabled timer with its new clock value.
	switch (new_clock) {
		case 0: // CPU clock / 1024
			// TIMA increments every time DIV % 4 == 0
			//-----------------------------------------------------------------
			// Number of cycles before DIV % 4 == 0:
			// * The sum of cycles for yet-to-queue DIV events:
			//   Number of unqueued DIV events: (3 - (DIV % 4))
			//   Multiply by cycles-per-event: CYC_DIV
			// * Add number of cycles before currently queued DIV
			//   event fires to the previous value.
			uint8_t div = gb_mem_direct_read(core, IO_DIV);
			EV(SCHEV_TIMA).until =
				((3 - (div % 4)) * CYC_DIV) + EV(SCHEV_DIV).until;
			//-----------------------------------------------------------------
			break;
		case 1: // CPU clock / 16
			// TIMA increments every time DIV's cycle counter % CYC_TIMA[1] (4) == 0
			EV(SCHEV_TIMA).until = EV(SCHEV_DIV).until % CYC_TIMA[1];
			break;
		case 2: // CPU clock / 64
			// TIMA increments every time DIV's cycle counter % CYC_TIMA[2] (16) == 0
			EV(SCHEV_TIMA).until = EV(SCHEV_DIV).until % CYC_TIMA[2];
			break;
		case 3: // CPU clock / 256
			// TIMA increments every time DIV increments.
			EV(SCHEV_TIMA).until = EV(SCHEV_DIV).until;
			break;
	} // switch (new_clock)
	insert_event(core, SCHEV_TIMA);
} // end gb_sch_on_tac_update()

//=======================================================================
// def gb_sch_on_lcdc_update()
void
gb_sch_on_lcdc_update(
		struct gb_core* restrict core,
		uint8_t old_lcdc, uint8_t new_lcdc) {
	// The only control bit which affects scheduling is whether or not
	// the PPU is enabled.
	old_lcdc &= IO_LCDC_PPU_ENABLED;
	new_lcdc &= IO_LCDC_PPU_ENABLED;
	if (old_lcdc == new_lcdc)
		return; // No scheduler changes.
	
	if (!new_lcdc) // PPU is turning off
		pause_event(core, SCHEV_PPU);
	else // PPU is turning on
		insert_event(core, SCHEV_PPU);
} // end gb_sch_on_lcdc_update()

//=======================================================================
//-----------------------------------------------------------------------
// Internal function definitions
//-----------------------------------------------------------------------
//=======================================================================
// def execute_event()
// TODO: Execute a single event multiple times efficently.
// Time passing rapidly enough to warrant optimization
// can occur when HALTed or STOPped.
// TODO: During HALT or STOP, only events that trigger
// interrupts would matter.
static void
execute_event(struct gb_core* restrict core) {
	// 1. Pop event from front of event queue.
	// 2. Execute event.
	// 3. Increment event cycles-before-firing counter by
	//    the number of cycles before it fires again.
	// 4. Re-insert event into event queue.
	enum gb_schev_id event = pop_event(core);
	switch (event) {
		case SCHEV_PPU:
			EV(SCHEV_PPU).until += CYC_PPU[gb_mem_io_advance_ppu(core)];
			break;
		case SCHEV_DIV:
			gb_mem_io_increment_div(core);
			EV(SCHEV_DIV).until += CYC_DIV;
			break;
		case SCHEV_TIMA:
			gb_mem_io_increment_tima(core);
			EV(SCHEV_TIMA).until +=
				CYC_TIMA[gb_mem_direct_read(core, IO_TAC) & IO_TAC_CLOCK_SELECT];
			break;
		case SCHEV_SERIAL: // TODO: Not implemented
			return; // Not reinserted
		case SCHEV_TIMELIMIT: // TODO: Not implemented
			break;
		default:
			// TODO: Error
			break;
	} // end switch (event)
	insert_event(core, event);
} // end execute_event()

static inline enum gb_schev_id
pop_event(struct gb_core* restrict core) {
	// Undo relative adjustment to event following first event.
	if (EV_FIRST.next != SCHEV_NONE)
		EV(EV_FIRST.next).until += EV_FIRST.until;
	// Connect header to event following first event.
	enum gb_schev_id popped = EV_HEADER.next;
	EV_HEADER.next = EV(popped).next;
	return popped;
} // end pop_event()

static inline void
remove_event(struct gb_core* restrict core, enum gb_schev_id event) {
	// Must find the event in the queue before removing it.
	// The event pointing to it needs to be directed to the event
	// following it.
	enum gb_schev_id prev = SCHEV_HEADER;
	while (EV(prev).next != SCHEV_NONE) {
		if (EV(prev).next == event) {
			EV(prev).next = EV(event).next; // prev.next = prev.next.next
			EV(event).next = SCHEV_DISABLED;
			return;
		}
	} // end iteration through queued events
} // end remove_event()

//=======================================================================
// doc insert_event()
// Inserts a pre-prepared event into the list of active events.
//
// Before calling this function, the _absolute_ number of cycles
// before `target` fires (independent of any active events) must
// be assigned to `target`.until. This function will convert this
// value to be relative to other active events in the list.
//
// Behavior is undefined if `target` is already a member of the list
// of active events when calling this function.
//-----------------------------------------------------------------------
// Parameters:
// * core
//   The emulation core which owns the scheduler whose active event
//   list will be inserted into.
// * target
//   The ID of the event to insert.
//=======================================================================
// def insert_event()
static void
insert_event(struct gb_core* restrict core, enum gb_schev_id target) {
	// Iterate through the list of active events.
	// `target` is to be inserted immediately before the first event
	// in the list that fires after `target` fires.
	// (The first event whose `until` value > `target`.until)
	//
	// For each event that fires before `target`, subtract that
	// event's remaining cycles before firing from `target`'s remaining
	// cycles.
	// TODO: Optimize out `curr` (just use EV(prev).next)
	enum gb_schev_id prev = SCHEV_HEADER;
	enum gb_schev_id curr = EV_HEADER.next;
	while (curr != SCHEV_NONE // SCHEV_NONE indicates end-of-list
	    && EV(target).until > EV(curr).until) {
		EV(target).until -= EV(curr).until;
		prev = curr;
		curr = EV(curr).next;
	}

	// Insert `target` between `prev` and `curr`.
	// Subtract `target`'s cycles-before-firing from `curr`'s
	// cycles-before-firing.
	EV(prev).next = target;
	EV(target).next = curr;
	if (curr != SCHEV_NONE)
		EV(curr).until -= EV(target).until;
} // end insert_event()

static inline uint8_t
clock_bit_state(struct gb_core* restrict core, uint8_t tac_clock_speed) {
	// Note: DIV bits in comments refer to the full 16-bit DIV register.
	//       Only the high 8 bits of DIV are visible to the program.
	//       Bits are labeled from 0-15, with 0 being the least-significant
	//       bit, and 15 being the most-significant bit.
	//
	// Also note that the emulation ignores the lowest 2 inaccessible DIV
	// bits for the purposes of scheduling.
	switch (tac_clock_speed) {
		case 0: // DIV bit 9 (bit 1 of DIV register)
			return gb_mem_direct_read(core, IO_DIV) & 0x2;
		// Remaining cases are for inaccessible DIV bits.
		// Their values are acquired by subtracting the remaining
		// cycles before next DIV incrementation from the total number
		// of cycles between DIV incrementations.
		// The bit chosen is shifted over by 2, as the lowest 2
		// inaccessible DIV bits are not emulated.
		case 1: // DIV bit 3
			return (CYC_DIV - EV(SCHEV_DIV).until) & 0x02;
		case 2: // DIV bit 5
			return (CYC_DIV - EV(SCHEV_DIV).until) & 0x08;
		case 3: // DIV bit 7
			return (CYC_DIV - EV(SCHEV_DIV).until) & 0x20;
	} // end switch (tac_clock_speed)
} // end clock_bit_state()

//=======================================================================
// doc pause_event()
// TODO
//=======================================================================
// def pause_event()
static inline void
pause_event(struct gb_core* restrict core, enum gb_schev_id target) {
	LOGT("begin: core=%p, target=%d", core, target);
	assert(target < NUM_SCHEVS);
	assert(target != SCHEV_HEADER);
	if (EV(target).next == SCHEV_DISABLED)
		return; // target event not running

	// Locate `target` event in the active event queue, and accumulate
	// the absolute number of cycles before `target` fires.
	// At the end of this loop, `prev` should be the event BEFORE `target`.
	int16_t target_abs_until = EV(target).until;
	enum gb_schev_id prev = SCHEV_HEADER;
	while (EV(prev).next != SCHEV_NONE && EV(prev).next != target) {
		LOGT("inside loop: prev=%d, target_abs_util=%" PRId16, prev, target_abs_until);
		prev = EV(prev).next;
		target_abs_until += EV(prev).until;
	}
	LOGT("after loop: prev=%d, prev.next=%d, target_abs_until=%" PRId16,
			prev, EV(prev).next, target_abs_until);
	if (EV(prev).next == SCHEV_NONE) {
		// Target event not found, but should be running.
		// This is a programming error.
		assert(0);
		return;
	}

	// Event's position in the queue is known and its cycle
	// count has been made absolute. Disconnect `target` from the queue.
	if (EV(target).next != SCHEV_NONE)
		EV(EV(target).next).until += EV(target).until; // target.next.until += target.until
	EV(prev).next = EV(target).next; // prev.next = target.next
	EV(target).next = SCHEV_DISABLED;
	EV(target).until = target_abs_until;
	LOGT("returning");
} // end pause_event()

//static inline enum gb_schev_index
//find_prev_event(
//		struct gb_core* restrict core,
//		enum gb_schev_index target_event) {
//	if (core->sch.next == target_event)
//		return SCHEV_NONE; // Front event, has no previous event.
//
//	enum gb_schev_index current_event = core->sch.next;
//	while (current_event != SCHEV_NONE) {
//		if (core->sch.ev[current_event].next == target_event)
//			return current_event;
//		event =
//	} // end iteration through core->sch.ev
//	return SCHEV_NONE; // Event is not queued, has no previous event.
//} // end find_prior_event()

