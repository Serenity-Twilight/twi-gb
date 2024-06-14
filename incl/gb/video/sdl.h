#ifndef GB_VIDEO_SDL_H
#define GB_VIDEO_SDL_H
#include <SDL.h>

struct gb_video_sdl {
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;
}; // end struct gb_video_sdl

struct gb_video_sdl_params {
	char* restrict window_title;
	struct {
		int width;
		int height;
	} window;
	struct {
		int width;
		int height;
	} output;
}; // end struct gb_video_sdl_params

int gb_video_sdl_init(
		struct gb_video_sdl* restrict vid,
		const struct gb_video_sdl_params* restrict params);

void
gb_video_sdl_destroy(struct gb_video_sdl* restrict vid);

void*
gb_video_sdl_start_drawing(struct gb_video_sdl* restrict vid, int* pitch);
int
gb_video_sdl_finish_drawing(struct gb_video_sdl* restrict vid);
int
gb_video_sdl_draw_clear(struct gb_video_sdl* restrict vid);

#endif // GB_VIDEO_SDL_H

