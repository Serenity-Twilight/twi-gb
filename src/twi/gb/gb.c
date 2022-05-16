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
//=======================================================================
#include <threads.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_video.h>
#include <twi/gb/gb.h>
#include <twi/gb/log.h>
#include <twi/std/assertf.h>

//=======================================================================
//-----------------------------------------------------------------------
// Private function declarations
//-----------------------------------------------------------------------
//=======================================================================
static uint_fast8_t handle_events(struct twi_gb*);
static void handle_window_event(struct twi_gb*, SDL_Event* event);

//=======================================================================
//-----------------------------------------------------------------------
// Public function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def twi_gb_run()
//=======================================================================
void
twi_gb_run(struct twi_gb* gb) {
	struct timespec slp_time = {.tv_sec=0, .tv_nsec=15000000L};
	size_t color_offset = 0;
	uint8_t* vram = (uint8_t*)twi_gb_mem_read_sector(&(gb->mem), TWI_GB_MEM_SECTOR_VRAM);
	for (;;) {
		thrd_sleep(&slp_time, NULL);
		if (!handle_events(gb))
			return;
		for (size_t i = 0, pos = 0; i < 160; ++i, pos += 4)
			*((uint32_t*)(vram + pos)) = (color_offset + i) % 1536;
		if ((color_offset += 4) >= 1536)
			color_offset -= 1536;
		twi_gb_ppu_draw(&(gb->ppu), &(gb->mem));
	} // end infinite loop
} // end twi_gb_run()

//=======================================================================
//-----------------------------------------------------------------------
// Private function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// decl handle_events()
// def handle_events()
// TODO
//=======================================================================
static uint_fast8_t
handle_events(struct twi_gb* gb) {
	twi_assert_notnull(gb);

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_WINDOWEVENT: handle_window_event(gb, &event); break;
			case SDL_QUIT: return 0;
		} // end determination of event type
	} // end polling of pending events
	
	return 1;
} // end handle_events()

//=======================================================================
// decl handle_window_event()
// def handle_window_event()
// TODO
//=======================================================================
static void
handle_window_event(struct twi_gb* gb, SDL_Event* event) {
	twi_assert_notnull(gb);
	twi_assert_notnull(event);
	// TODO: twi_assert_eq(event->window.type, SDL_WINDOWEVENT);

	switch (event->window.event) {
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			// TODO: twi_assert_eq(event->window.windowID, SDL_GetWindowID(gb->ppu.window));
			twi_gb_ppu_onchange_resolution(&(gb->ppu));
			break;
	} // end determination of specific window event
} // end handle_window_event()

