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
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_video.h>
#include <twi/gb/gb.h>
#include <twi/gb/log.h>
#include <twi/std/assert.h>

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
	while (handle_events(gb));
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
			// TODO: twi_assert_eq(event->window.windowID, SDL_GetWindowID(gb->vid.window));
			twi_gb_vid_onchange_resolution(&(gb->vid));
			break;
	} // end determination of specific window event
} // end handle_window_event()

