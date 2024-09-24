#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <threads.h>
#include <time.h>
#include <SDL.h>
#include "gb/core.h"
#include "gb/cpu.h"
#include "gb/cpu/interpreter.h"
#include "gb/log.h"
#include "gb/mem.h"
#include "gb/pad.h"
#include "gb/ppu.h"
#include "gb/sch.h"
#define PRX_TRUNCATE_PREFIX 1
#include "prx/timespec.h"

#undef GB_LOG_MAX_LEVEL
#define GB_LOG_MAX_LEVEL LVL_INF

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL CONSTANT DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================
enum {
	// Actually 0.0167427062988... seconds-per-frame
	// (59.7 frames-per-second)
	NSEC_PER_FRAME = 16742706
};

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL TYPE DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================
struct input_state {
	uint8_t pad;
	uint8_t fast_forward;
};

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL FUNCTION DECLARATIONS
//-----------------------------------------------------------------------
//=======================================================================
static int
handle_event(SDL_Event* restrict event, struct input_state* restrict state);
static void
handle_keydown(const SDL_KeyboardEvent* restrict kevent, struct input_state* restrict input);
static void
handle_keyup(const SDL_KeyboardEvent* restrict kevent, struct input_state* restrict input);

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL FUNCTION DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def gb_core_init()
uint8_t
gb_core_init(struct gb_core* restrict core) {
	gb_cpu_init(core);
	if (gb_mem_init(core))
		return 1;
	gb_sch_init(core);
	return 0;
} // end gb_core_init()

//=======================================================================
// def gb_core_run()
void
gb_core_run(
		struct gb_core* restrict core,
		struct gb_ppu* restrict ppu) {
	assert(!SDL_InitSubSystem(SDL_INIT_EVENTS)); // TODO

	struct timespec next_frame_start_time;
	if (timespec_get(&next_frame_start_time, TIME_UTC) != TIME_UTC) {
		LOGF("timespec_get() failure.");
		return;
	}
	struct input_state input = {.pad=gb_pad_init(), .fast_forward=0};
	LOGT("enter main loop");
	while (1) {
		// Execute
		gb_cpu_interpret_frame(core);
		gb_mem_copy_ppu_state(core, &(ppu->state));
		if (gb_dmg_draw(ppu))
			LOGF("gb_dmg_draw() failure");

		// Host event handling
		uint8_t old_pad = input.pad;
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (handle_event(&event, &input)) {
				SDL_QuitSubSystem(SDL_INIT_EVENTS);
				return;
			}
		} // end event polling
#undef GB_LOG_MAX_LEVEL
#define GB_LOG_MAX_LEVEL LVL_TRC
		LOGD("pad=0x%02X -> pad=0x%02X", old_pad, input.pad);
#undef GB_LOG_MAX_LEVEL
#define GB_LOG_MAX_LEVEL LVL_INF
		gb_core_set_pad(core, input.pad);

		// Add total frame time to the start time of this frame to get
		// the start time of the next frame.
		timespec_add_nsec(&next_frame_start_time, NSEC_PER_FRAME);
		// Calculate time between now and when
		// the next frame should execute:
		struct timespec now;
		if (timespec_get(&now, TIME_UTC) != TIME_UTC) {
			LOGF("timespec_get() failure.");
			return;
		}
		
		if (!input.fast_forward && timespec_cmp(&next_frame_start_time, &now) >= 0) {
			// Normal (frame-capped) speed, and ahead of or on schedule.
			// Sleep until the next frame:
			struct timespec sleep_duration;
			timespec_sub(&sleep_duration, &next_frame_start_time, &now);
			LOGT("Sleeping for {.sec=%lld,.nsec=%lld}",
					sleep_duration.tv_sec, sleep_duration.tv_nsec);
			int sleep_result;
			while (sleep_result = thrd_sleep(&sleep_duration, &sleep_duration)) {
				LOGD("Sleep interrupted (%d). Remaining={.sec=%lld,.nsec=%lld}",
						sleep_result, sleep_duration.tv_sec, sleep_duration.tv_nsec);
				if (sleep_result < -1) {
					LOGF("thrd_sleep() error.");
					return;
				}
			} // end while (sleep interrupted)
			LOGT("wake up");
		} else {
			// Either:
			// 1. Fast forwarding, want to go as fast as possible, or
			// 2. Behind schedule, and need to skip sleeping
			// Either way, begin the next frame NOW.
			next_frame_start_time = now;
		}
	} // end while (1)
} // end gb_core_run()

void
gb_core_set_pad(struct gb_core* restrict core, uint8_t gb_pad) {
	gb_mem_set_pad(core, gb_pad);
} // end gb_core_update_pad()

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL FUNCTION DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// doc handle_event()
// TODO
//=======================================================================
// def handle_event()
static int
handle_event(SDL_Event* restrict event, struct input_state* restrict input) {
	switch(event->type) {
		case SDL_KEYDOWN:
			handle_keydown(&(event->key), input);
			break;
		case SDL_KEYUP:
			handle_keyup(&(event->key), input);
			break;
		case SDL_WINDOWEVENT:
			if (event->window.event == SDL_WINDOWEVENT_CLOSE)
				return 1;
			break;
	}
	return 0;
} // end handle_event()

//=======================================================================
// doc handle_keydown()
// TODO
//=======================================================================
// def handle_keydown()
static void
handle_keydown(const SDL_KeyboardEvent* restrict kevent, struct input_state* restrict input) {
	switch (kevent->keysym.sym) {
		case SDLK_f:
			input->fast_forward = 1;
			break;
		case SDLK_RIGHT:
			input->pad = gb_pad_press(input->pad, GBPAD_RIGHT);
			break;
		case SDLK_LEFT:
			input->pad = gb_pad_press(input->pad, GBPAD_LEFT);
			break;
		case SDLK_UP:
			input->pad = gb_pad_press(input->pad, GBPAD_UP);
			break;
		case SDLK_DOWN:
			input->pad = gb_pad_press(input->pad, GBPAD_DOWN);
			break;
		case SDLK_z:
			input->pad = gb_pad_press(input->pad, GBPAD_A);
			break;
		case SDLK_x:
			input->pad = gb_pad_press(input->pad, GBPAD_B);
			break;
		case SDLK_RSHIFT:
			input->pad = gb_pad_press(input->pad, GBPAD_SELECT);
			break;
		case SDLK_RETURN:
			input->pad = gb_pad_press(input->pad, GBPAD_START);
			break;
	} // end switch()
} // end handle_keydown()

//=======================================================================
// doc handle_keyup()
// TODO
//=======================================================================
// def handle_keyup()
static void
handle_keyup(const SDL_KeyboardEvent* restrict kevent, struct input_state* restrict input) {
	switch (kevent->keysym.sym) {
		case SDLK_f:
			input->fast_forward = 0;
			break;
		case SDLK_RIGHT:
			input->pad = gb_pad_release(input->pad, GBPAD_RIGHT);
			break;
		case SDLK_LEFT:
			input->pad = gb_pad_release(input->pad, GBPAD_LEFT);
			break;
		case SDLK_UP:
			input->pad = gb_pad_release(input->pad, GBPAD_UP);
			break;
		case SDLK_DOWN:
			input->pad = gb_pad_release(input->pad, GBPAD_DOWN);
			break;
		case SDLK_z:
			input->pad = gb_pad_release(input->pad, GBPAD_A);
			break;
		case SDLK_x:
			input->pad = gb_pad_release(input->pad, GBPAD_B);
			break;
		case SDLK_RSHIFT:
			input->pad = gb_pad_release(input->pad, GBPAD_SELECT);
			break;
		case SDLK_RETURN:
			input->pad = gb_pad_release(input->pad, GBPAD_START);
			break;
	} // end switch()
} // end handle_keyup()

