#include <SDL2/SDL.h>
#include <twi/gb/gb.h>
#include <twi/gb/log.h>
#include <twi/gb/mem.h>
#include <twi/gb/mode.h>
#include <twi/gb/ppu.h>

int main(int argc, char* argv[]) {
	twi_gb_log_create();
	struct twi_gb gb;
	twi_gb_mem_init(&(gb.mem), NULL, NULL, TWI_GB_MODE_DMG);
	if (!twi_gb_ppu_init(&(gb.ppu))) {
		twi_gb_run(&gb);
		twi_gb_ppu_destroy(&(gb.ppu));
	}
	
//	LOGF("This is a fatal message, id = %d", TWI_GB_LOG_LEVEL_FATAL);
//	LOGE("This is an error message, id = %d", TWI_GB_LOG_LEVEL_ERROR);
//	LOGW("This is a warning message, id = %d", TWI_GB_LOG_LEVEL_WARN);
//	LOGI("This is an informational message, id = %d", TWI_GB_LOG_LEVEL_INFO);
//	LOGD("This is a debug message, id = %d", TWI_GB_LOG_LEVEL_DEBUG);
//	LOGT("This is a trace message, id = %d", TWI_GB_LOG_LEVEL_TRACE);
//	LOGR("This is a rom error message, id = %d", TWI_GB_LOG_LEVEL_ROMERR);
	SDL_Quit();
	twi_gb_log_delete();
}
