// TODO: File description. Probably need one for source file, too.
#ifndef TWI_LOG_H
#define TWI_LOG_H

#include <stdint.h>
#include <stdio.h>

//======================================================================
// Logging levels for twi_log.
//
// The logging level value assigned to a twi_log stream shall be a
// bitwise OR combination of all logging levels that should be output
// to the associated stream.
//
// For example: If a particular stream should only receive debug and
// trace messages, then the logging level should be:
// level = TWI_LOG_LEVEL_DEBUG | TWI_LOG_LEVEL_TRACE;
//
// Levels:
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
//   information useful for validating program logic.
//
// - TWI_LOG_LEVEL_TRACE: Logging level for messages containing very
//   specific state and position information used for tracing the
//   execution path taken by the process.
//======================================================================
#define TWI_LOG_LEVEL_FATAL 0x01
#define TWI_LOG_LEVEL_ERROR 0x02
#define TWI_LOG_LEVEL_WARN 0x04
#define TWI_LOG_LEVEL_INFO 0x08
#define TWI_LOG_LEVEL_DEBUG 0x10
#define TWI_LOG_LEVEL_TRACE 0x20

//======================================================================
// In addition to the stream-specific logging level, whether or not
// a certain level of messages is written to a stream is also dependent
// on the following bitmask.
//
// For each message logged, the following mask is applied:
// TWI_LOG_LEVEL_MASK & {stream-specific level} & {message level}
//
// If the resulting value is non-zero, the message is written to the stream.
// Otherwise, the message is ignored.
//
// The purpose of this bitmask macro is to allow for disabling of
// specific loggging levels at compile-time, allowing the compiler to
// optimize out attempts to log at particular levels, eliminating the
// cost of attempting to write log messages which no stream has the
// level for. It also serves the purpose of simplifying disabling logging
// from specific translation units, reducing clutter from messages
// produced by code which is confirmed to be functioning.
//
// If the mask is not defined, it defaults to accepting all logging
// levels.
//======================================================================
#ifndef TWI_LOG_LEVEL_MASK
#	define TWI_LOG_LEVEL_MASK \
		TWI_LOG_LEVEL_FATAL | TWI_LOG_LEVEL_ERROR | TWI_LOG_LEVEL_WARN | \
		TWI_LOG_LEVEL_INFO | TWI_LOG_LEVEL_DEBUG | TWI_LOG_LEVEL_TRACE
#endif

// The size of the narrow-width character buffer used when
// generating pathnames via twi_log_stream_open_timestamped() and
// twi_log_stream_open_indexed(). Note that twi_log_stream_open()
// uses the user-provided pathname without any modification, and
// therefore this buffer size has no effect on that function.
#ifndef TWI_LOG_PATHNAME_BUFSZ
#	define TWI_LOG_PATHNAME_BUFSZ 4096
#endif

//======================================================================
// decl twi_log_status
// def twi_log_status
//
// Enumation of status codes returned by twi_log functions that can
// fail.
//
// To aid in the programmatic identification of errors, the following
// guarantees are provided:
// 1. The enumeration equivalent to 0 (false in a boolean context) will
//    be considered a successful status.
// 2. Enumerations with integer values > 0 are programmer errors, triggered
//    by erroneous arguments. Note that error codes are only provided
//    for non-obvious user errors that can reasonably occur unexpectedly.
//    Especially egregious programming errors are checked instead by
//    asserts, which will halt the process with debugging information
//    when encountered. If asserts are disabled via TWI_LOG_NOASSERT,
//    these errors will instead go unchecked and result in undefined
//    behavior.
// 3. Enumerations with integer values < 0 indicate errors outside of
//    the control of the process and/or programmer, such as file
//    management-related failures. Note that twi_log may use Standard C
//    and/or POSIX libraries in order to operate. Whether or not `errno`
//    is set when such functions fail is implementation defined. Twi_log
//    functions will _always_ clear the value of `errno` before usage
//    of a function that can fail which may set `errno`. In implementations
//    in which `errno` is set, it can be used to acquire additional
//    information with regards to the failed operation.
//======================================================================
enum twi_log_status {
	TWI_LOG_STATUS_CANNOT_OPEN_STREAM = -2,
	TWI_LOG_STATUS_CANNOT_ALLOCATE_MEMORY = -1,
	TWI_LOG_STATUS_OK = 0
} // end enum twi_log_status

//======================================================================
// decl struct twi_log_stream
//======================================================================
struct twi_log_stream;

//======================================================================
// decl struct twi_log
// def struct twi_log
//----------------------------------------------------------------------
// Logging object. Contains references and metadata for formatting
// and writing log messages.
//----------------------------------------------------------------------
// Members:
//----------------------------------------------------------------------
// * out (INTERNAL):
// Output streams to which messages are written.
//----------------------------------------------------------------------
// * implicit_path:
// Implicit pathname prefix omitted from log messages to preserve
// horizontal space.
//
// If implicit_path points to a null-terminated character array,
// filepaths provided with log messages will have the implicit
// path truncated from the front of the filepath written into the
// log if and only if the filepath is prefixed with characters
// matching all of the characters of the implicit path (excluding
// null terminator). See examples below for further explanation.
//
// If implicit_path points to NULL, this behavior is disabled and all
// filepaths are written in their entirety to their associated log
// message.
//
// If implicit_path points to anything else, behavior is undefined.
//
// Examples:
// - If the implicit path is "src/", and the filepath is
//   "src/main.c", the filepath will appear as "main.c" in the log
//   message.
// - If the implicit path is "src" and the filepath is
//   "src/main.c", the filepath will appear as "/main.c" in the log
//   message.
// - If the implicit path is "src/a" and the filepath is
//   "src/main.c", the filepath will appear as "src/main.c" in the
//   log message.
// - If the implicit path is "main.c" and the filepath is
//   "src/main.c", the filepath will appear as "src/main.c" in the
//   log message.
//
// Twi_log will only ever read from the implicit_path variable, never
// write to it, except during initialization. At initialization,
// implicit_path is assigned NULL. The value of implicit_path affects
// how filepath will appear in the output of all open streams associated
// with the twi_log.
//----------------------------------------------------------------------
// * epoch (INTERNAL):
// Timestamp acquired upon the initialization of the twi_log.
// Used to derive relative timestamps used in log messages.
//----------------------------------------------------------------------
// * active_levels (INTERNAL):
// A bitmask indicating logging levels which are used by at least one
// of the twi_log's open streams.
//
// For example, if a twi_log holds two open streams, one accepting only
// TWI_LOG_LEVEL_TRACE and the other accepting only TWI_LOG_LEVEL_INFO,
// active_levels would be the bitwise OR combination of these two
// logging levels.
//
// This is done to allow for a quick way to discard log messages which
// will not be written to any held stream before the message undergoes
// formatting, thus reducing the overhead of ignored messages.
//----------------------------------------------------------------------
// * flags (INTERNAL):
// Internal state flags.
//======================================================================
struct twi_log {
	struct twi_log_stream* out;
	char* implicit_path;
	struct timespec epoch;
	uint8_t active_levels;
	uint8_t flags;
};

//======================================================================
// decl twi_log_err_libc
//----------------------------------------------------------------------
// If and only if an external twi_log function fails due to the failure
// of a libc (C Standard Library) function, this variable is set to a
// pointer to a statically-allocated constant narrow-width character
// array containing the name of the libc function which last failed
// on this thread from within a twi_log function.
//
// NOTE: One exception to this rule is twi_log_write(), which will
// instead attempt to report errors directly to stderr without
// informing the caller. This is done to prevent the caller from
// expressing concern as to the success of twi_log_write(), so
// as to prevent the need for a test at every point at which
// twi_log_write() is called.
//
// When a thread begins, this variable is initialized to NULL,
// but twi_log functions will only set the variable on libc error.
// Under no circumstances will any twi_log function clear this
// variable back to NULL, though user code is free to do so.
//======================================================================
_Thread_local extern const char* twi_log_err_libc;

//======================================================================
// decl twi_log_init()
//----------------------------------------------------------------------
// Initializes the twi_log pointed to by `log`.
//
// Attempting to use a twi_log with any other twi_log function prior
// to initialization or attempting to initialize a single twi_log
// more than once without a call to twi_log_destroy() before each
// subsequent reinitialization will result in undefined behavior.
//----------------------------------------------------------------------
// Parameters:
// * log
// Pointer to an uninitialized twi_log.
//======================================================================
void
twi_log_init(struct twi_log* log);

//======================================================================
// decl twi_log_destroy()
//----------------------------------------------------------------------
// Releases the resources held by the twi_log pointed to by `log`,
// putting it into an unusable state and making the memory occupied by
// the twi_log safe to release or reinitialize using twi_log_init().
//
// If twi_log_destroy() is called on a twi_log which has yet to be
// initialized or has been destroyed previously without reinitialization,
// behavior is undefined.
//----------------------------------------------------------------------
// Parameters:
// * log
// Pointer to a previously initialized twi_log.
//======================================================================
void
twi_log_destroy(struct twi_log* log);

//======================================================================
// decl twi_log_stream_open()
//----------------------------------------------------------------------
// Opens the file specified by `pathname` and sets it as the new
// target output stream of `log`.
//
// If another stream is already open in `log`, it will be closed
// before attempting to open the new stream.
//
// If this function fails to open a new output stream at `pathname`,
// it will return with a non-zero value and `log` will be left without
// an open output stream. If `log` already contained an open output
// stream before the call, it will have been closed.
//----------------------------------------------------------------------
// Parameters:
// * log
// Pointer to an initialized twi_log.
// * pathname
// Pointer to a null-terminated character array containing the pathname
// at which to open the new ouput stream.
//----------------------------------------------------------------------
// Returns:
// 0 on success
// -1 if a libc function fails, in which case twi_log_err_libc is
// set to the name of the failing function. If the libc implementation
// sets errno for that function, then errno is available. If not, then
// errno is cleared (= 0).
//======================================================================
int_fast8_t
twi_log_stream_open(
		struct twi_log* log,
		const char* pathname
);

//======================================================================
// decl twi_log_stream_open_timestamped()
// TODO
//======================================================================
int_fast8_t
twi_log_stream_open_timestamped(
		struct twi_log* log,
		const char* prefix,
		const char* suffix
);

//======================================================================
// decl twi_log_stream_open_indexed()
// TODO
//======================================================================
int_fast8_t
twi_log_stream_open_indexed(
		struct twi_log* log,
		const char* prefix,
		const char* suffix,
		uint8_t max_files
);

//======================================================================
// decl twi_log_stream_close()
//----------------------------------------------------------------------
// Closes the internal target output stream used by `log`.
//
// If no internal target output stream is open in `log` at the time
// that this function is called, then this function does nothing.
//----------------------------------------------------------------------
// Parameters:
// - log
// Pointer to an initialized twi_log.
//======================================================================
void
twi_log_stream_close(struct twi_log* log);

//======================================================================
// decl twi_log_write()
//
// General-purpose logging function for twi_log.
//
// If lvl is less than or equal to the compile-time global max logging
// level, as well as the logging level of the provided log, a
// caller-defined message is written to the output stream managed by
// the provided log. If these conditions are not fulfilled, then this
// function does nothing.
//
// When the aforementioned conditions are fulfilled, the following is
// written to the output stream in the listed order:
// 1. A timestamp, in the format of [seconds.microseconds].
// 2. An abbreviation indicating the message's logging level.
// 3. The file and line in which this function was invoked by the caller.
// 4. The caller's message, with the variadic arguments subsituted into
//    the null-terminated character array pointed to by msg in a fashion
//    alike to that of the printf family of functions.
//
// Parameters:
//----------------------------------------------------------------------
// - struct twi_log log
// An initialized twi_log.
//----------------------------------------------------------------------
// - enum twi_log_level lvl
// The log level of the message to write.
// Can be any of value of TWI_LOG_LEVEL except TWI_LOG_LEVEL_NONE.
//----------------------------------------------------------------------
// - const char* msg
// Pointer to a null-terminated character string, containing a number
// of printf-alike substitutions equal in number to the number of
// variadic arguments.
//----------------------------------------------------------------------
// - ...
// Variadic argument list, matching the number and respective types of
// the substitution escape sequences in the null-terminated character
// array pointed to by msg.
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
#define twi_log_write(log, lvl, msg, ...) \
	if ((lvl) > TWI_LOG_LEVEL_NONE \
	 && (lvl) <= TWI_LOG_LEVEL_GLOBAL_MAX \
	 && (lvl) <= (log).max_lvl) { \
		twi_log_write( \
				&(log), \
				lvl, \
				__FILE__, \
				__LINE__, \
				msg, \
				##__VA_ARGS__ \
		); \
	}

// Shortcut macros for TWI_LOG which include logging level in the
// macro name, instead of the arguments.
#define TWI_LOGF(log, msg, ...) \
	twi_log_write(log, TWI_LOG_LEVEL_FATAL, msg, ##__VA_ARGS__)
#define TWI_LOGE(log, msg, ...) \
	twi_log_write(log, TWI_LOG_LEVEL_ERROR, msg, ##__VA_ARGS__)
#define TWI_LOGW(log, msg, ...) \
	twi_log_write(log, TWI_LOG_LEVEL_WARN, msg, ##__VA_ARGS__)
#define TWI_LOGI(log, msg, ...) \
	twi_log_write(log, TWI_LOG_LEVEL_INFO, msg, ##__VA_ARGS__)
#define TWI_LOGD(log, msg, ...) \
	twi_log_write(log, TWI_LOG_LEVEL_DEBUG, msg, ##__VA_ARGS__)
#define TWI_LOGT(log, msg, ...) \
	twi_log_write(log, TWI_LOG_LEVEL_TRACE, msg, ##__VA_ARGS__)

#endif // TWI_LOG_H
