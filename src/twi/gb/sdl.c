//=======================================================================
//-----------------------------------------------------------------------
// src/twi/gb/sdl.c
// Written by Serenity Twilight
//-----------------------------------------------------------------------
//=======================================================================
#include <stdint.h>
//#include <time.h>
#include <twi/std/assert.h>
#include <twi/gb/log.h>
#include <twi/gb/mem.h>
#include <twi/gb/sdl.h>

//=======================================================================
//-----------------------------------------------------------------------
// Internal global variables
//-----------------------------------------------------------------------
//=======================================================================

static const uint8_t tex_width = 160;
static const uint8_t tex_height = 144;

//=======================================================================
//-----------------------------------------------------------------------
// External function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def twi_gb_sdlvid_init()
//=======================================================================
uint_fast8_t
twi_gb_sdlvid_init(struct twi_gb_sdlvid* vid) {
	twi_assert_notnull(vid);

	// Allocate temporary pointers so that `vid` won't be changed in
	// the case of failure.
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;

	if (!SDL_WasInit(SDL_INIT_VIDEO)) {
		if (SDL_Init(SDL_INIT_VIDEO)) {
			LOGF("Failed to initialize SDL Video subsystem with the following error: %s", SDL_GetError());
			return 1;
		}
	}

	// TODO: User-defined window dimensions.
	if (window = SDL_CreateWindow("twi-gb", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, tex_width*4, tex_height*4, 0)) {
		LOGF("Failed to create a window with the following error: %s", SDL_GetError());
		return 1;
	}

	// TODO: Look into/experiment with custom-selecting a renderer.
	if (renderer = SDL_CreateRenderer(window, -1, 0)) {
		LOGF("Failed to create a renderer with the following error: %s", SDL_GetError());
		goto failure_after_window;
	}

	// Set renderer-clearing color to black.
	if (SDL_SetRenderDrawColor(renderer,0,0,0,SDL_ALPHA_OPAQUE)) {
		LOGF("Failed to set the renderer's draw color with the following error: %s", SDL_GetError());
		goto failure_after_renderer;
	}

	// TODO: Look into/experiment with alternate pixel formats.
	if (texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, tex_width, tex_height)) {
		LOGF("Failed to create a texture with the following error: %s", SDL_GetError());
		goto failure_after_renderer;
	}

	vid->window = window;
	vid->renderer = renderer;
	vid->texture = texture;
	vid->pixels = NULL; // Indicates an unlocked texture.
	return 0;

failure_after_renderer:
	SDL_DestroyRenderer(renderer);
failure_after_window:
	SDL_DestroyWindow(window);
	return 1;
} // end twi_gb_sdlvid_init()

//=======================================================================
// def twi_gb_sdlvid_destroy()
//=======================================================================
void
twi_gb_sdlvid_destroy(struct twi_gb_sdlvid* vid) {
	twi_assert_notnull(vid);

	if (vid->texture != NULL)
		SDL_DestroyTexture(vid->texture);

	if (vid->renderer != NULL)
		SDL_DestroyRenderer(vid->renderer);

	if (vid->window != NULL)
		SDL_DestroyWindow(vid->window);
} // end twi_gb_sdlvid_destroy()

//=======================================================================
// def twi_gb_sdlvid_get_drawing_area()
//=======================================================================
uint32_t*
twi_gb_sdlvid_get_drawing_area(struct twi_gb_sdlvid* vid) {
	twi_assert_notnull(vid);
	twi_assert_notnull(vid->texture);
	
	// Lock the texture if it isn't already.
	if (vid->pixels == NULL) {
		// Pitch isn't needed as the texture size is known.
		// Acquire it to keep SDL happy, then discard it.
		int pitch;
		if (SDL_LockTexture(vid->texture, NULL, (void**)&(vid->pixels), &pitch)) {
			vid->pixels = NULL; // Just in case SDL changed it anyway.
			LOGE("Unable to lock SDL texture. Reason: %s", SDL_GetError());
			return NULL;
		}

		// Ensure 32-bit alignment, which I'm not yet sure SDL guarantees.
		// 32-bit alignment allows for a pixel's color to be set with a
		// single instruction, as opposed to four, and thus is quite a bit
		// faster.
		if ((uintptr_t)(vid->pixels) % sizeof(uint32_t) != 0)
			LOGW("Texture pixel destination is NOT aligned for 32-bit access (%p)!", vid->pixels);
		else
			LOGD("Texture pixel destination is aligned for 32-bit access (%p).", vid->pixels);
	}

	return vid->pixels;
} // end twi_gb_sdlvid_get_drawing_area()

//=======================================================================
// def twi_gb_sdlvid_draw()
//=======================================================================
uint_fast8_t
twi_gb_sdlvid_draw(struct twi_gb_sdlvid* vid) {
	twi_assert_notnull(vid);
	twi_assert_notnull(vid->renderer);
	twi_assert_notnull(vid->texture);

	// If the texture is still locked, then there's nothing new to draw.
	if (vid->pixels == NULL) {
		LOGD("Redundant draw call. Texture isn't locked.");
		return 0;
	}

	SDL_UnlockTexture(vid->texture);
	vid->pixels = NULL;
	if (SDL_RenderClear(vid->renderer)) {
		LOGE("Unable to clear the renderer. Reason: %s", SDL_GetError());
		return 1;
	}
	// TODO: 4th argument will have to specify a region of the renderer
	//       to draw to, otherwise aspect ratio can be broken.
	if (SDL_RenderCopy(vid->renderer, vid->texture, NULL, NULL)) {
		LOGE("Unable to copy texture to renderer. Reason: %s", SDL_GetError());
		return 1;
	}
	SDL_RenderPresent(vid->renderer);

	return 0;
} // end twi_gb_sdlvid_draw()

//=======================================================================
// def twi_gb_sdl_draw()
//=======================================================================
//void
//twi_gb_sdl_draw(struct twi_gb_sdl* sdl, struct twi_gb_mem* mem) {
//	twi_assert_notnull(sdl);
//	twi_assert_notnull(mem);
//
//	SDL_RenderClear(sdl->renderer);
//	SDL_RenderPresent(sdl->renderer);
//} // end twi_gb_sdl_draw()

//=======================================================================
// def twi_gb_sdl_draw_test()
//=======================================================================
//void
//twi_gb_sdl_draw_test(struct twi_gb_sdl* sdl) {
//	twi_assert_notnull(sdl);
//
//	// Write the word "TEST", with each character being 40 pixels wide
//	// and 40 pixels tall. There will be no pixels between letters.
//	// Each pixel requires 4 bytes for RGBA color.
//	static const uint8_t scale = 40;
//	static const uint32_t black = 0x000000FF;
//	static const uint32_t white = 0xFFFFFFFF;
//	uint32_t test[40][160];
//	// Color the background white.
//	for (uint8_t h = 0; h < 40; ++h)
//		for (uint8_t w = 0; w < 160; ++w)
//			test[h][w] = white;
//
//	// T
//	for (uint8_t h = 0; h < 4; ++h)
//		for (uint8_t w = 0; w < 40; ++w)
//			test[h][w] = black;
//	for (uint8_t h = 4; h < 40; ++h)
//		for (uint8_t w = 18; w < 22; ++w)
//			test[h][w] = black;
//
//	// E
//	for (uint8_t h = 0; h < 40; ++h)
//		for (uint8_t w = 40; w < 44; ++w)
//			test[h][w] = black;
//	for (uint8_t offset = 0; offset < 40; offset += 18)
//		for (uint8_t h = offset; h < offset+4; ++h)
//			for (uint8_t w = 44; w < 80; ++w)
//				test[h][w] = black;
//
//	// S
//	for (uint8_t offset = 0; offset < 40; offset += 18)
//		for (uint8_t h = offset; h < offset+4; ++h)
//			for (uint8_t w = 80; w < 120; ++w)
//				test[h][w] = black;
//	for (uint8_t w_offset = 80, h_offset = 0; w_offset < 120; w_offset += 36, h_offset += 18)
//		for (uint8_t h = h_offset+4; h < h_offset+18; ++h)
//			for (uint8_t w = w_offset; w < w_offset+4; ++w)
//				test[h][w] = black;
//
//	// T
//	for (uint8_t h = 0; h < 4; ++h)
//		for (uint8_t w = 120; w < 160; ++w)
//			test[h][w] = black;
//	for (uint8_t h = 4; h < 40; ++h)
//		for (uint8_t w = 138; w < 142; ++w)
//			test[h][w] = black;
//
//	time_t start = time(NULL);
//	time_t end = start+10;
//	struct timespec prev;
//	timespec_get(&prev, TIME_UTC);
//	struct timespec now = prev;
//
//	uint8_t pos = 0;
//	while (difftime(end, now.tv_sec) > 0.0) {
//		timespec_get(&now, TIME_UTC);
//		if (difftime(now.tv_sec, prev.tv_sec) + (now.tv_nsec/1000000000.0 - prev.tv_nsec/1000000000.0) > 0.01667) {
//			++pos;
//			prev = now;
//			if (pos+40 >= 144)
//				pos = 0;
//		}
//		uint32_t* pixels;
//		int pitch;
//		SDL_LockTexture(sdl->texture, NULL, (void**)(&pixels), &pitch);
//
//		for (uint8_t h = 0; h < pos; ++h)
//			for (uint8_t w = 0; w < 160; ++w)
//				pixels[h*160 + w] = white;
//		for (uint8_t h = pos; h < pos+40; ++h)
//			for (uint8_t w = 0; w < 160; ++w)
//				pixels[h*160+w] = test[h-pos][w];
//		for (uint8_t h = pos+40; h < 144; ++h)
//			for (uint8_t w = 0; w < 160; ++w)
//				pixels[h*160 + w] = white;
//
//		SDL_UnlockTexture(sdl->texture);
//		SDL_RenderCopy(sdl->renderer, sdl->texture, NULL, NULL);
//		SDL_RenderPresent(sdl->renderer);
//	}
//	
//} // end twi_gb_sdl_draw_test()

