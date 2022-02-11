//=======================================================================
//=======================================================================
// src/twi/gb/sdl.c
// Written by Serenity Twilight
//=======================================================================
//=======================================================================
#include <stdint.h>
#include <time.h>
#include <twi/std/assert.h>
#include <twi/gb/log.h>
#include <twi/gb/sdl.h>

static const uint8_t tex_width = 160;
static const uint8_t tex_height = 144;

//=======================================================================
// def twi_gb_sdl_init()
//=======================================================================
uint_fast8_t
twi_gb_sdl_init(struct twi_gb_sdl* sdl) {
	twi_assert_notnull(sdl);
	sdl->window = NULL;
	sdl->renderer = NULL;

	if (SDL_Init(SDL_INIT_VIDEO)) {
		LOGF("Failed to initialize SDL Video subsystem with the following error: %s", SDL_GetError());
		return 1;
	}

	if ((sdl->window = SDL_CreateWindow("twi-gb", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, tex_width*4, tex_height*4, 0)) == NULL) {
		LOGF("Failed to create a window with the following error: %s", SDL_GetError());
		return 1;
	}

	if ((sdl->renderer = SDL_CreateRenderer(sdl->window, -1, 0)) == NULL) {
		LOGF("Failed to create a renderer with the following error: %s", SDL_GetError());
		return 1;
	}

	if (SDL_SetRenderDrawColor(sdl->renderer,0,0,0,SDL_ALPHA_OPAQUE)) {
		LOGF("Failed to set the renderer's draw color with the following error: %s", SDL_GetError());
		return 1;
	}

	if ((sdl->texture = SDL_CreateTexture(sdl->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, tex_width, tex_height)) == NULL) {
		LOGF("Failed to create a texture with the following error: %s", SDL_GetError());
		return 1;
	}

	return 0;
} // end twi_gb_sdl_init()

//=======================================================================
// def twi_gb_sdl_destroy()
//=======================================================================
void
twi_gb_sdl_destroy(struct twi_gb_sdl* sdl) {
	twi_assert_notnull(sdl);

	if (sdl->texture != NULL)
		SDL_DestroyTexture(sdl->texture);

	if (sdl->renderer != NULL)
		SDL_DestroyRenderer(sdl->renderer);

	if (sdl->window != NULL)
		SDL_DestroyWindow(sdl->window);

	SDL_Quit();
} // end twi_gb_sdl_destroy()

//=======================================================================
// def twi_gb_sdl_draw()
//=======================================================================
void
twi_gb_sdl_draw(struct twi_gb_sdl* sdl) {
	twi_assert_notnull(sdl);

	SDL_RenderClear(sdl->renderer);
	SDL_RenderPresent(sdl->renderer);
} // end twi_gb_sdl_draw()

//=======================================================================
// def twi_gb_sdl_draw_test()
//=======================================================================
void
twi_gb_sdl_draw_test(struct twi_gb_sdl* sdl) {
	twi_assert_notnull(sdl);

	// Write the word "TEST", with each character being 40 pixels wide
	// and 40 pixels tall. There will be no pixels between letters.
	// Each pixel requires 4 bytes for RGBA color.
	static const uint8_t scale = 40;
	static const uint32_t black = 0x000000FF;
	static const uint32_t white = 0xFFFFFFFF;
	uint32_t test[40][160];
	// Color the background white.
	for (uint8_t h = 0; h < 40; ++h)
		for (uint8_t w = 0; w < 160; ++w)
			test[h][w] = white;

	// T
	for (uint8_t h = 0; h < 4; ++h)
		for (uint8_t w = 0; w < 40; ++w)
			test[h][w] = black;
	for (uint8_t h = 4; h < 40; ++h)
		for (uint8_t w = 18; w < 22; ++w)
			test[h][w] = black;

	// E
	for (uint8_t h = 0; h < 40; ++h)
		for (uint8_t w = 40; w < 44; ++w)
			test[h][w] = black;
	for (uint8_t offset = 0; offset < 40; offset += 18)
		for (uint8_t h = offset; h < offset+4; ++h)
			for (uint8_t w = 44; w < 80; ++w)
				test[h][w] = black;

	// S
	for (uint8_t offset = 0; offset < 40; offset += 18)
		for (uint8_t h = offset; h < offset+4; ++h)
			for (uint8_t w = 80; w < 120; ++w)
				test[h][w] = black;
	for (uint8_t w_offset = 80, h_offset = 0; w_offset < 120; w_offset += 36, h_offset += 18)
		for (uint8_t h = h_offset+4; h < h_offset+18; ++h)
			for (uint8_t w = w_offset; w < w_offset+4; ++w)
				test[h][w] = black;

	// T
	for (uint8_t h = 0; h < 4; ++h)
		for (uint8_t w = 120; w < 160; ++w)
			test[h][w] = black;
	for (uint8_t h = 4; h < 40; ++h)
		for (uint8_t w = 138; w < 142; ++w)
			test[h][w] = black;

	time_t start = time(NULL);
	time_t end = start+10;
	struct timespec prev;
	timespec_get(&prev, TIME_UTC);
	struct timespec now = prev;

	uint8_t pos = 0;
	while (difftime(end, now.tv_sec) > 0.0) {
		timespec_get(&now, TIME_UTC);
		if (difftime(now.tv_sec, prev.tv_sec) + (now.tv_nsec/1000000000.0 - prev.tv_nsec/1000000000.0) > 0.01667) {
			++pos;
			prev = now;
			if (pos+40 >= 144)
				pos = 0;
		}
		uint32_t* pixels;
		int pitch;
		SDL_LockTexture(sdl->texture, NULL, (void**)(&pixels), &pitch);

		for (uint8_t h = 0; h < pos; ++h)
			for (uint8_t w = 0; w < 160; ++w)
				pixels[h*160 + w] = white;
		for (uint8_t h = pos; h < pos+40; ++h)
			for (uint8_t w = 0; w < 160; ++w)
				pixels[h*160+w] = test[h-pos][w];
		for (uint8_t h = pos+40; h < 144; ++h)
			for (uint8_t w = 0; w < 160; ++w)
				pixels[h*160 + w] = white;

		SDL_UnlockTexture(sdl->texture);
		SDL_RenderCopy(sdl->renderer, sdl->texture, NULL, NULL);
		SDL_RenderPresent(sdl->renderer);
	}
	
} // end twi_gb_sdl_draw_test()

