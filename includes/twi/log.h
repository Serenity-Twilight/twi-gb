#ifndef TWI_LOG_H
#define TWI_LOG_H

#include <stdint.h>
#include <time.h>

//======================================================================
// decl enum twi_log_level
// def enum twi_log_level
//
// Logging levels for twi_log, with lower levels indicating more
// urgent messages, and higher levels indicating messages which are
// intended more for informational purposes.
//======================================================================
enum twi_log_level {
	// Pseudo logging level. A log with TWI_LOG_LEVEL_NONE set as its
	// max level will never be written to. A log message with its level
	// indicated as TWI_LOG_LEVEL_NONE will never be written. A
	// TWI_LOG_GLOBAL_MAX_LEVEL of TWI_LOG_LEVEL_NONE will disable all
	// logging throughout the entire program.
	TWI_LOG_LEVEL_NONE,
	// Logging level indicating fatal errors from which the program cannot
	// be reasonably expected to recover.
	TWI_LOG_LEVEL_FATAL,
	// Logging level for errors in execution which are not necessarily
	// or immediately fatal, but still require urgent attention.
	TWI_LOG_LEVEL_ERROR,
	// Logging level for warning of abnormal running conditions which
	// may or may not indicate an erroneous state. Warnings should be
	// issued in valid but unusual circumstances
	// (Example: missing configuration file, which will always happen
	// on first run or after configuration has been deleted).
	TWI_LOG_LEVEL_WARN,
	// Logging level for reporting information which may be useful in
	// normal operation, such as state changes or progress reports.
	TWI_LOG_LEVEL_INFO,
	// Logging level for debug messages.
	TWI_LOG_LEVEL_DEBUG,
	// Logging level for debug messages which aid in tracking the
	// execution path of the program, such as reporting function entries
	// and exits, and what arguments are passed.
	TWI_LOG_LEVEL_TRACE
};

// Defines the maximum logging level to acknowledge at compile time.
// Used to optimize out higher logging levels, which are likely to
// occur more frequently and be more verbose, out of the program
// during compilation, removing their impact on performance.
// If no max level is specified, then all levels are enabled.
#ifndef TWI_LOG_LEVEL_GLOBAL_MAX
#	define TWI_LOG_LEVEL_GLOBAL_MAX TWI_LOG_LEVEL_TRACE
#endif

//======================================================================
// decl struct twi_log
// def struct twi_log
//
// twi_log state struct
//======================================================================
struct twi_log {
	// Output stream written to by TWI_LOG().
	FILE* target;
	// Filename prefix to trim from incoming filenames.
	// Any filename written to the log whose prefix matches the
	// contents of the character array pointed to by trim_filename_prefix
	// will be removed. Used to remove implicit directories from listed
	// filenames, saving line width. Note that the entirety of
	// trim_filename_prefix must match the start of a filename string
	// for trimming to occur.
	//
	// For example, if TWI_LOG is invoked from within the file
	// "src/main.c", and trim_filename_prefix = "src/", the filename
	// will be logged as "main.c". If trim_filename_prefix = "src", then
	// the filename would appear as "/main.c". If
	// trim_filename_prefix = "src//", then the filename is unchanged.
	const char* trim_filename_prefix;
	// INTERNAL USE ONLY
	// The timestamp at which a twi_log object was initialized.
	// Used for calculating timestamps.
	struct timespec epoch;
	// The maximum twi_log_level whose messages that a particular
	// twi_log will log. Any message whose twi_log_level is less than
	// or equal to this level (assuming that the level does not exceed
	// TWI_LOG_LEVEL_GLOBAL_MAX and is not TWI_LOG_LEVEL_NONE) will be
	// logged, and any other value will be ignored.
	uint8_t max_level;
	// INTERNAL USE ONLY
	// Internal state flags. Described in source file.
	uint8_t flags;
};

//======================================================================
// decl TWI_LOG()
//
// General-purpose logging macro for struct twi_log.
//
// If lvl is less than or equal to the compile-time global max logging
// level as well as the logging level of the twi_log pointed to by
// log_ptr, the null-terminated character array pointed to by msg
// is written to the output stream log_ptr->target (if
// log_ptr->target is not NULL) with substitution upon the contents of
// the null-terminated character array pointed to by msg alike to that
// of stdio's printf family of functions using the variadic argument
// list following msg.
//
// The contents of msg is prefixed with a timestamp indicating the
// time since the initialization of the twi_log pointed to by
// log_ptr, the human-readable format of the logging level, the
// file name and line number in which TWI_LOG was invoked, and 
// The contents of msg are suffixed by a line feed.
//
// The timestamp used is of the following format:
// days:hours:minutes:seconds:milliseconds
// All values of the timestamp, with the exception of days, is
// written with a fixed width to improve horizontal alignment of
// log messages and therefore improve readability.
//
// If lvl is greater than either the global max logging level or the
// max logging level of the twi_log pointed to by log_ptr, lvl is
// equal to TWI_LOG_LEVEL_NONE, log_ptr points to NULL, or
// log_ptr->target points to NULL,  then TWI_LOG does nothing.
//
// Parameters:
// * struct twi_log* log_ptr - Pointer to an initialized twi_log to
//   write to the file streams of.
// * enum twi_log_level lvl - One of the values of twi_log_level,
//   excluding TWI_LOG_LEVEL_NONE.
// * const char* msg - Pointer to a null-terminated character string,
//   containing a number of printf-alike substitutions equal in number
//   to the number of variadic arguments.
// * ... - Variadic argument list, matching number and respective types
//   to the substitution escape sequences in msg.
//======================================================================
#define TWI_LOG(log_ptr, lvl, msg, ...) \
	if (lvl != TWI_LOG_LEVEL_NONE \
	 && lvl <= TWI_LOG_LEVEL_GLOBAL_MAX \
	 && log_ptr && log_ptr->target && lvl <= log_ptr->max_level) { \
		twi_log_write( \
				log_ptr, \
				lvl, \
				__FILE__, \
				__LINE__, \
				msg, \
				##__VA_ARGS__ \
		); \
	}

// Shortcut macros for TWI_LOG which include logging level in the
// macro name, instead of the arguments.
#define TWI_LOGF(log_ptr, msg, ...) \
	TWI_LOG(log_ptr, TWI_LOG_LEVEL_FATAL, msg, ##__VA_ARGS__)
#define TWI_LOGE(log_ptr, msg, ...) \
	TWI_LOG(log_ptr, TWI_LOG_LEVEL_ERROR, msg, ##__VA_ARGS__)
#define TWI_LOGW(log_ptr, msg, ...) \
	TWI_LOG(log_ptr, TWI_LOG_LEVEL_WARN, msg, ##__VA_ARGS__)
#define TWI_LOGI(log_ptr, msg, ...) \
	TWI_LOG(log_ptr, TWI_LOG_LEVEL_INFO, msg, ##__VA_ARGS__)
#define TWI_LOGD(log_ptr, msg, ...) \
	TWI_LOG(log_ptr, TWI_LOG_LEVEL_DEBUG, msg, ##__VA_ARGS__)
#define TWI_LOGT(log_ptr, msg, ...) \
	TWI_LOG(log_ptr, TWI_LOG_LEVEL_TRACE, msg, ##__VA_ARGS__)

//======================================================================
// decl twi_log_write()
//
// DO NOT CALL THIS FUNCTION DIRECTLY!
// Instead, invoke this function via the TWI_LOG macro, or one of its
// log level-specific variants. 
//
// For optimization reasons, log level checking is done at the call
// site via the aforementioned macros. Invoking this function directly
// ignores maximum log level configuration and requires the caller to
// be responsible for providing additional callsite metadata.
//
// This interface is to be considered PRIVATE, and is liable to
// change in ways that will invalidate any code which invokes it
// directly.
//======================================================================
void
twi_log_write(
		struct twi_log*,
		enum twi_log_level,
		const char*,
		long,
		const char*,
		...
);

#endif // TWI_LOG_H
