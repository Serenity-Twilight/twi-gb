#include <SDL.h>
#define GB_LOG_MAX_LEVEL LVL_TRC
#include "gb/log.h"
#include "gb/video/sdl.h"

void log_sdl_error(const char* restrict funcname) {
	LOGE("%s(): %s", funcname, SDL_GetError());
} // end log_sdl_error()

//=======================================================================
// def gb_video_sdl_init()
int gb_video_sdl_init(
		struct gb_video_sdl* restrict vid,
		const struct gb_video_sdl_params* restrict params) {
	//-------------------------------------------------------
	// Initialize video subsystem
	if (SDL_InitSubSystem(SDL_INIT_VIDEO)) {
		log_sdl_error("SDL_InitSubSystem");
		return 1;
	}

	//-------------------------------------------------------
	// Create window with caller-requested size and title.
	vid->window = SDL_CreateWindow(
			params->window_title,
			SDL_WINDOWPOS_CENTERED, // x-position
			SDL_WINDOWPOS_CENTERED, // y-position
			params->window.width, params->window.height,
			SDL_WINDOW_HIDDEN); // flags
	if (vid->window == NULL) {
		log_sdl_error("SDL_CreateWindow");
		goto quit_subsystem;
	}

	//-------------------------------------------------------
	// Create hardware-accelerated renderer
	vid->renderer = SDL_CreateRenderer(
			vid->window, -1, SDL_RENDERER_ACCELERATED);
	if (vid->renderer == NULL) {
		log_sdl_error("SDL_CreateRenderer");
		goto destroy_window;
	}
	// Set clear color to black
	if (SDL_SetRenderDrawColor(vid->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE) < 0) {
		log_sdl_error("SDL_SetRenderDrawColor");
		goto destroy_renderer;
	}
	// Clear garbage that's currently in the renderer client area.
	if (SDL_RenderClear(vid->renderer)) {
		log_sdl_error("SDL_RenderClear");
		goto destroy_renderer;
	}
	SDL_RenderPresent(vid->renderer);

	//-------------------------------------------------------
	// Create a single texture that will be used to buffer drawing
	// pixels to the renderer's backbuffer.
	vid->texture = SDL_CreateTexture(
			vid->renderer, SDL_PIXELFORMAT_RGBA32,
			SDL_TEXTUREACCESS_STREAMING, // texture will be updated constantly
			params->output.width, params->output.height);
	if (vid->texture == NULL) {
		log_sdl_error("SDL_CreateTexture");
		goto destroy_renderer;
	}

	//-------------------------------------------------------
	// Now that the window has been prepared, make it visible.
	SDL_ShowWindow(vid->window);
	return 0;
	//-------------------------------------------------------
	// Cleanup on failure:
destroy_renderer:
	SDL_DestroyRenderer(vid->renderer);
destroy_window:
	SDL_DestroyWindow(vid->window);
quit_subsystem:
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	return 1;
} // end gb_video_sdl_init()

//=======================================================================
// def gb_video_sdl_destroy()
void
gb_video_sdl_destroy(struct gb_video_sdl* restrict vid) {
	SDL_DestroyTexture(vid->texture);
	SDL_DestroyRenderer(vid->renderer);
	SDL_DestroyWindow(vid->window);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
} // end gb_video_sdl_destroy()

//=======================================================================
// def gb_video_sdl_start_drawing()
void*
gb_video_sdl_start_drawing(struct gb_video_sdl* restrict vid, int* pitch) {
	int throwaway_pitch;
	if (pitch == NULL)
		pitch = &throwaway_pitch;

	void* pixels;
	if (SDL_LockTexture(vid->texture, NULL, &pixels, pitch)) {
		log_sdl_error("SDL_LockTexture");
		return NULL;
	}
	return pixels;
} // end gb_video_sdl_start_drawing()

//=======================================================================
// def gb_video_sdl_finish_drawing()
int
gb_video_sdl_finish_drawing(struct gb_video_sdl* restrict vid) {
	SDL_UnlockTexture(vid->texture);
	if (SDL_RenderClear(vid->renderer)) {
		log_sdl_error("SDL_RenderClear");
		return 1;
	}
	if (SDL_RenderCopy(vid->renderer, vid->texture, NULL, NULL)) {
		log_sdl_error("SDL_RenderCopy");
		return 1;
	}
	SDL_RenderPresent(vid->renderer);
	return 0;
} // end gb_video_sdl_finish_drawing()

//=======================================================================
// def gb_video_sdl_draw_clear()
int
gb_video_sdl_draw_clear(struct gb_video_sdl* restrict vid) {
	if (SDL_RenderClear(vid->renderer)) {
		log_sdl_error("SDL_RenderClear");
		return 1;
	}
	SDL_RenderPresent(vid->renderer);
	return 0;
} // end gb_video_sdl_draw_clear()

