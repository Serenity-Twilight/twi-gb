#include <stdio.h>
#include <SDL.h>
#include "gb/core.h"
#include "gb/core/typedef.h"
#include "gb/pak.h"
#include "gb/mem.h"
#include "gb/ppu.h"

static void
print_usage();

int main(int argc, char* argv[]) {
	int status = 0;
	if (argc < 2) {
		// ROM filepath not provided.
		print_usage(argc >= 1 ? argv[0] : "unknown");
		status = 1;
		goto end_of_function;
	}

	struct gb_ppu ppu;
	if (gb_ppu_init(&ppu)) {
		status = 1;
		goto end_of_function;
	}

	struct gb_pak* pak = gb_pak_create(argv[1]);
	if (pak == NULL) {
		status = 1;
		goto end_of_function;
	}

	struct gb_core core;
	if (gb_core_init(&core)) {
		gb_ppu_destroy(&ppu);
		status = 1;
		goto end_of_function;
	}
	gb_core_swap_pak(&core, pak);
	gb_core_run(&core, &ppu);

end_of_function:
	SDL_Quit();
	return status;
}

static void
print_usage(const char* restrict program_name) {
	printf("Usage:\n\t%s <ROM-filepath>\n", program_name);
} // end print_usage()

