#ifndef TWI_GB_LOG_H
#define TWI_GB_LOG_H

// TODO: Functionality to disable specific logging levels at compile
// time, thereby eliminating the performance overhead of checking
// for unused levels.

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
#define LOG(lvl, msg, ...) \
	if (twi_gb_log != NULL) {twi_log_write(twi_gb_log, lvl, msg, ##__VA_ARGS__)}
#define LOGF(msg, ...) LOG(TWI_GB_LOG_LEVEL_FATAL, msg, ##__VA_ARGS__)
#define LOGE(msg, ...) LOG(TWI_GB_LOG_LEVEL_ERROR, msg, ##__VA_ARGS__)
#define LOGW(msg, ...) LOG(TWI_GB_LOG_LEVEL_WARN, msg, ##__VA_ARGS__)
#define LOGI(msg, ...) LOG(TWI_GB_LOG_LEVEL_INFO, msg, ##__VA_ARGS__)
#define LOGD(msg, ...) LOG(TWI_GB_LOG_LEVEL_DEBUG, msg, ##__VA_ARGS__)
#define LOGT(msg, ...) LOG(TWI_GB_LOG_LEVEL_TRACE, msg, ##__VA_ARGS__)
#define LOGR(msg, ...) LOG(TWI_GB_LOG_LEVEL_ROMERR, msg, ##__VA_ARGS__)

uint_fast8_t twi_gb_log_create();
void twi_gb_log_delete();

#endif // TWI_GB_LOG_H

