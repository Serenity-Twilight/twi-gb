#include <stdio.h>
#include <SDL.h>
#include "gb/core.h"
#include "gb/core/typedef.h"
#include "gb/mem.h"
#include "gb/ppu.h"

static void
print_usage();

int main(int argc, char* argv[]) {
	if (argc < 2) {
		// ROM filepath not provided.
		print_usage(argc >= 1 ? argv[0] : "unknown");
		return 1;
	}

	struct gb_ppu ppu;
	if (gb_ppu_init(&ppu))
		return 1;

	struct gb_core core;
	gb_mem_rom_filepath = argv[1];
	if (gb_core_init(&core)) {
		gb_ppu_destroy(&ppu);
		return 1;
	}
	gb_core_run(&core, &ppu);

	SDL_Quit();
	return 0;
}

static void
print_usage(const char* restrict program_name) {
	printf("Usage:\n\t%s <ROM-filepath>\n", program_name);
} // end print_usage()

