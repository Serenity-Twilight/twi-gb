#ifndef TWI_LOG_H
#define TWI_LOG_H

#include <stdint.h>
#include <stdio.h>

//======================================================================
// decl enum twi_log_level
// def enum twi_log_level
//
// Logging levels for twi_log, with lower levels indicating more
// urgent messages, and higher levels indicating messages which are
// intended more for informational purposes.
//
// Values:
// - TWI_LOG_LEVEL_NONE: Pseudo-logging level. A log with
//   TWI_LOG_LEVEL_NONE as its max level will discard all logging
//   messages. A log message with level TWI_LOG_LEVEL_NONE will always
//   be discarded. A TWI_LOG_LEVEL_GLOBAL_MAX of TWI_LOG_LEVEL_NONE
//   will erase all logging operations from the compiled program.
//
// - TWI_LOG_LEVEL_FATAL: Logging level indicating errors from which
//   the process cannot recover, and termination is the only option.
//
// - TWI_LOG_LEVEL_ERROR: Logging level indicating errors which are
//   non-fatal.
//
// - TWI_LOG_LEVEL_WARN: Logging level indicating the detection of
//   unusual circumstances which are not incorrect or impossible,
//   but uncommon and may indicate a larger error.
//
// - TWI_LOG_LEVEL_INFO: Logging level for messages which inform the
//   user of normal state changes and operation progress.
//
// - TWI_LOG_LEVEL_DEBUG: Logging level for messages containing
//   information useful for discovering logic errors in the application.
//
// - TWI_LOG_LEVEL_TRACE: Logging level for messages containing very
//   specific state and position information used for tracing the
//   execution path taken by the process.
//======================================================================
enum twi_log_level {
	TWI_LOG_LEVEL_NONE,
	TWI_LOG_LEVEL_FATAL,
	TWI_LOG_LEVEL_ERROR,
	TWI_LOG_LEVEL_WARN,
	TWI_LOG_LEVEL_INFO,
	TWI_LOG_LEVEL_DEBUG,
	TWI_LOG_LEVEL_TRACE
};

// Defines the maximum logging level to acknowledge at compile time.
// Used to remove higher logging level messages during compilation,
// which are likely to occur more frequently and therefore have a
// greater impact on performance.
//
// If no max level is specified, then all levels are enabled.
#ifndef TWI_LOG_LEVEL_GLOBAL_MAX
#	define TWI_LOG_LEVEL_GLOBAL_MAX TWI_LOG_LEVEL_TRACE
#endif

//======================================================================
// decl struct twi_log
// def struct twi_log
//
// Logging object. Contains references and metadata for formatting
// and writing log messages.
//
// Members:
//----------------------------------------------------------------------
// - stream (INTERNAL):
// Output stream and the target of all log messages.
//----------------------------------------------------------------------
// - implicit_fp:
// Implicit filepath prefix omitted from log messages to preserve
// horizontal space.
//
// If `implicit_fp` points to a null-terminated character array,
// filepaths provided with log messages will have the implicit
// filepath truncated from the front of the filepath written into the
// log if and only if the filepath is prefixed with characters
// matching all of the characters of the implicit filepath (excluding
// null terminator). See examples below for further explanation.
//
// If `implicit_fp` points to NULL, this behavior is disabled and all
// filepaths are written in their entirety to their associated log
// message.
//
// If `implicit_fp` points to anything else, behavior is undefined.
//
// Examples:
// - If the implicit filepath is "src/", and the real filepath is
//   "src/main.c", the filepath will appear as "main.c" in the log
//   message.
// - If the implicit filepath is "src" and the real filepath is
//   "src/main.c", the filepath will appear as "/main.c" in the log
//   message.
// - If the implicit filepath is "src/a" and the real filepath is
//   "src/main.c", the filepath will appear as "src/main.c" in the
//   log message.
// - If the implicit filepath is "main.c" and the real filepath is
//   "src/main.c", the filepath will appear as "src/main.c" in the
//   log message.
//----------------------------------------------------------------------
// - epoch (INTERNAL):
// Timestamp acquired upon the initialization of the twi_log.
// Used to derive relative timestamps used in log messages.
//----------------------------------------------------------------------
// - max_lvl:
// The maximum level of incoming logs which will be
// written to `stream`. Logs whose levels are higher than this value
// will be discarded.
//
// See enum twi_log_level for a complete list of logging levels and
// each's intended use.
//----------------------------------------------------------------------
// - flags (INTERNAL):
// Internal state flags.
//======================================================================
struct twi_log {
	FILE* stream;
	char* implicit_fp;
	struct timespec epoch;
	uint8_t max_lvl;
	uint8_t flags;
};

//======================================================================
// decl twi_log_init()
//
// Initializes the twi_log pointed to by `log` with maximum logging
// level of `max_lvl`.
//
// This function should be called exactly once on a twi_log before
// proceeding with any other operations on that twi_log. Attempting to
// use an uninitialized twi_log or attempting to initialize a single
// twi_log more than once without a call to twi_log_destroy between
// each pair of initializations will result in undefined behavior.
//
// Parameters:
//----------------------------------------------------------------------
// - log
// Pointer to an uninitialized twi_log.
//----------------------------------------------------------------------
// - max_lvl
// The value to be set as `log`'s maximum allowed logging level.
// See the definition of enum twi_log_level for additional information.
//======================================================================
void
twi_log_init(
		struct twi_log* log,
		enum twi_log_level max_lvl
);

//======================================================================
// decl twi_log_destroy()
//
// Releases the resources held by the twi_log pointed to by `log`,
// putting it into an unusable state and making the memory occupied by
// the twi_log safe to release or reinitialize using twi_log_init().
//
// If twi_log_destroy() is called on a twi_log which has yet to be
// initialized or has been destroyed previously without reinitialization,
// behavior is undefined.
//
// Parameters:
//----------------------------------------------------------------------
// - log
// Pointer to a previously initialized twi_log.
//======================================================================
void
twi_log_destroy(struct twi_log* log);

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
		&& (log_ptr) != NULL \
		&& lvl <= (log_ptr)->max_lvl) { \
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
// decl twi_log_stream_open()
//
// Opens the file specified by `filename` and sets it as the new
// target output stream of `log`.
//
// If another stream is already open in `log`, it will be closed
// before attempting to open a stream to the file `filename`.
//
// If this function fails to open a new output stream to `filename`,
// it will return with a non-zero value and `log` will be left without
// an open output stream. If `log` already contained an open output
// stream before the call, it will have been closed.
//
// Parameters:
//----------------------------------------------------------------------
// - log
// Pointer to an initialized twi_log.
//----------------------------------------------------------------------
// - filename
// Pointer to a null-terminated character array representative of the
// name of a file.
//----------------------------------------------------------------------
//
// Returns:
// 0 on success, non-zero on failure.
//======================================================================
int
twi_log_stream_open(
		struct twi_log* log,
		const char* filename
);

//======================================================================
// decl twi_log_stream_close()
//
// Closes the internal target output stream used by `log`.
//
// If no internal target output stream is open in `log` at the time
// that this function is called, then this function does nothing.
//
// Parameters:
//----------------------------------------------------------------------
// - log
// Pointer to an initialized twi_log.
//======================================================================
void
twi_log_stream_close(struct twi_log* log);

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
