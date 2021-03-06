//======================================================================
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
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <twi/gb/log.h>

//======================================================================
//======================================================================
// Internal function declarations
//======================================================================
//======================================================================

//======================================================================
//======================================================================
// External variable declarations
//======================================================================
//======================================================================
struct twi_log* twi_gb_log;

//======================================================================
//======================================================================
// External function definitions
//======================================================================
//======================================================================

//======================================================================
// def twi_gb_log_create()
//======================================================================
uint_fast8_t
twi_gb_log_create() {
	// Create underlying twi_log.
	twi_gb_log = twi_log_create(2, 7); // 2 streams, 7 logging levels
	if (twi_gb_log == NULL)
		return 1;

	// Define implicit path prefix and logging levels.
	twi_log_set_implicit_path_prefix(twi_gb_log, "src/twi/");
	twi_log_define_level(twi_gb_log, TWI_GB_LOG_LEVEL_FATAL, "FATAL", "FTL", "fa");
	twi_log_define_level(twi_gb_log, TWI_GB_LOG_LEVEL_ERROR, "ERROR", "ERR", "ea");
	twi_log_define_level(twi_gb_log, TWI_GB_LOG_LEVEL_WARN, "WARN", "WRN", "wa");
	twi_log_define_level(twi_gb_log, TWI_GB_LOG_LEVEL_INFO, "INFO", "INF", "ia");
	twi_log_define_level(twi_gb_log, TWI_GB_LOG_LEVEL_DEBUG, "DEBUG", "DBG", "da");
	twi_log_define_level(twi_gb_log, TWI_GB_LOG_LEVEL_TRACE, "TRACE", "TRC", "ta");
	twi_log_define_level(twi_gb_log, TWI_GB_LOG_LEVEL_ROMERR, "ROMERR", "ROM", "ra");

	// Open streams.
	if (twi_log_open_stream(twi_gb_log, 0, "stdout", 0, "a")) {
		twi_gb_log_delete();
		return 1;
	}
	if (twi_log_open_stream(twi_gb_log, 1, "./log", 1, "a")) {
		twi_gb_log_delete();
		return 1;
	}

	return 0;
} // end twi_gb_log_create()

//======================================================================
// def twi_gb_log_delete()
//======================================================================
void
twi_gb_log_delete() {
	twi_log_delete(twi_gb_log);
	twi_gb_log = NULL;
} // end twi_gb_log_delete()

