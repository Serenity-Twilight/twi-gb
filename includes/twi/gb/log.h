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
// Configuration and convenience macros for the application-wide
// shared log.
//-----------------------------------------------------------------------
//=======================================================================
#ifndef TWI_GB_LOG_H
#define TWI_GB_LOG_H

#include <stddef.h>
#include <stdint.h>
#include <twi/std/log.h>

//=======================================================================
// decl enum twi_gb_log_level
// def enum twi_gb_log_level
// 
// Enumeration of the logging levels defined and utilized by twi-gb.
//
// * FATAL:
// Used to report the encounter of error conditions in which recovery
// is impossible. Messages of this level should be immediately followed
// by graceful (if possible) cleanup and termination of the process.
//
// * ERROR:
// Used to report error conditions which, whilst often preventing the
// process from continuing the action which triggered the error, are
// not fatal and the process may recover from by aborting the action.
//
// * WARN:
// Used to report peculiar conditions which are possible and sometimes
// even correct, but suspicious enough to warn the user, in case a
// more subtle error has occurred. Warnings can typically be ignored
// with alternate actions following the encounter of the condition
// which caused the warning.
//
// * INFO:
// Used to report actions taken by the process which aids the user in
// tracing the process's execution, but is reserved for only major
// observable events in order to prevent too high of a performance
// overhead. The most common example would be to report a major state
// in change in the process, such as the loading of a file or completion
// of a lengthy operation.
//
// * DEBUG:
// Used to report miscellaneous information regarding process state for
// the purposes of the debugging the program. Suitable for more verbose
// (and therefore more performance-intensive) reporting than what is
// suitable for the INFO level, but does not fit the criteria to be
// considered for the TRACE level.
//
// * TRACE:
// Used to report very specific information regarding process state, in
// order to be able to precisely follow process execution. A debugging
// log level which is expected to have a high performance impact in
// exchange for greatly easing debugging.
//
// * ROMERR:
// Used to report any erroneous or unsafe behavior invoked by a running
// ROM image. Useful for debugging roms, but not necessarily the emulator
// itself.
//=======================================================================
enum twi_gb_log_level {
	TWI_GB_LOG_LEVEL_FATAL,
	TWI_GB_LOG_LEVEL_ERROR,
	TWI_GB_LOG_LEVEL_WARN,
	TWI_GB_LOG_LEVEL_INFO,
	TWI_GB_LOG_LEVEL_DEBUG,
	TWI_GB_LOG_LEVEL_TRACE,
	TWI_GB_LOG_LEVEL_ROMERR
};

//=======================================================================
// decl twi_gb_log
//
// The process-wide log handle for twi-gb.
//
// For simplification in possible refactoring, this variable should
// never be referenced by its identifier outside of the twi-gb log.h
// and log.c files. Instead, utilize the for-your-convenience logging
// macros defined below.
//=======================================================================
extern struct twi_log* twi_gb_log;

// Macros which simplify the callsite signature of log writes for
// programmer convenience.
#if TWI_GB_NOLOG
#	define LOG(...) ((void)0)
#else
# define LOG(lvl, msg, ...) \
	if (twi_gb_log != NULL) twi_log_write(twi_gb_log, lvl, msg, ##__VA_ARGS__)
#endif // end TWI_GB_NOLOG

#if TWI_GB_LOG_NOFATAL
#	define LOGF(...) ((void)0)
#else
#	define LOGF(msg, ...) LOG(TWI_GB_LOG_LEVEL_FATAL, msg, ##__VA_ARGS__)
#endif // TWI_GB_LOG_NOFATAL

#if TWI_GB_LOG_NOERROR
#	define LOGE(...) ((void)0)
#	define LOGCE(...) ((void)0)
#else
# define LOGE(msg, ...) LOG(TWI_GB_LOG_LEVEL_ERROR, msg, ##__VA_ARGS__)
#	include <errno.h>
#	include <string.h>
#	define LOGCE(msg, ...) \
	if (errno != 0) \
		LOGE(msg " (%s)", ##__VA_ARGS__, strerror(errno)); \
	else \
		LOGE(msg " (no errno)", ##__VA_ARGS__)
#endif // TWI_GB_LOG_NOERROR

#if TWI_GB_LOG_NOWARN
#	define LOGW(...) ((void)0)
#else
#	define LOGW(msg, ...) LOG(TWI_GB_LOG_LEVEL_WARN, msg, ##__VA_ARGS__)
#endif // TWI_GB_LOG_NOWARN

#if TWI_GB_LOG_NOINFO
#	define LOGI(...) ((void)0)
#else
#	define LOGI(msg, ...) LOG(TWI_GB_LOG_LEVEL_INFO, msg, ##__VA_ARGS__)
#endif // TWI_GB_LOG_NOINFO

#if TWI_GB_LOG_NODEBUG
#	define LOGD(...) ((void)0)
#else
#	define LOGD(msg, ...) LOG(TWI_GB_LOG_LEVEL_DEBUG, msg, ##__VA_ARGS__)
#endif // TWI_GB_LOG_NODEBUG

#if TWI_GB_LOG_NOTRACE
#	define LOGT(...) ((void)0)
#else
#	define LOGT(msg, ...) LOG(TWI_GB_LOG_LEVEL_TRACE, msg, ##__VA_ARGS__)
#endif // TWI_GB_LOG_NOTRACE

#if TWI_GB_LOG_NOROMERR
#	define LOGR(...) ((void)0)
#else
#	define LOGR(msg, ...) LOG(TWI_GB_LOG_LEVEL_ROMERR, msg, ##__VA_ARGS__)
#endif // TWI_GB_LOG_NOROMERR

//=======================================================================
// Assertions in twi-gb use a custom handler to write assertions to
// the global log, as well as to close the log before terminating.
//=======================================================================
#undef TWI_ASSERT_MSG_HANDLER // Silence compiler warning of macro redefinition.
#define TWI_ASSERT_MSG_HANDLER(msg, ...) { \
	LOGF("Assertion failure " msg, ##__VA_ARGS__); \
	twi_gb_log_delete(); \
}

// TODO: Give proper descriptions.
uint_fast8_t twi_gb_log_create();
void twi_gb_log_delete();

#endif // TWI_GB_LOG_H

