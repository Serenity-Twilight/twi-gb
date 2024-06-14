#include <stdio.h>
#include <SDL.h>
#include "gb/core.h"
#include "gb/ppu.h"

int main() {
	struct gb_ppu ppu;
	if (gb_ppu_init(&ppu))
		return 1;

	struct gb_core core;
	if (gb_core_init(&core)) {
		gb_ppu_destroy(&ppu);
		return 1;
	}
	gb_core_run(&core, &ppu);

	SDL_Quit();
	return 0;
}

