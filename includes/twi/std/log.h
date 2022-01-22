// TODO: File description. Probably need one for source file, too.
#ifndef TWI_LOG_H
#define TWI_LOG_H

#include <stdint.h>
#include <stdio.h>

//======================================================================
// def twi_log_errno
// TODO
//======================================================================
extern _Thread_local int twi_log_errno;

//======================================================================
// decl struct twi_log
// def struct twi_log
// TODO: Revise
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
struct twi_log;

//======================================================================
// decl twi_log_create()
// TODO
//======================================================================
struct twi_log*
(twi_log_create)(uint8_t num_streams, uint8_t num_levels);

//======================================================================
// decl twi_log_delete()
// TODO
//======================================================================
void
(twi_log_delete)(struct twi_log* log);

//======================================================================
// decl twi_log_open_stream()
// TODO
//======================================================================
uint_fast8_t
(twi_log_open_stream)(
		struct twi_log* log,
		uint8_t stream_id,
		const char* path,
		uint_fast8_t append,
		const char* level_codes
);

//======================================================================
// decl twi_log_close_stream()
// TODO
//======================================================================
void
(twi_log_close_stream)(
		struct twi_log* log,
		uint8_t stream_id
);

//======================================================================
// decl twi_log_set_stream_level()
// TODO
//======================================================================
void
(twi_log_set_stream_level)(
		struct twi_log* log,
		uint8_t stream_id,
		const char* level_codes
);

//======================================================================
// decl twi_log_define_level()
// TODO
//======================================================================
void
(twi_log_define_level)(
		struct twi_log* log,
		uint8_t level_id,
		const char* name,
		const char* abbrev,
		const char* codes
);

//======================================================================
// decl twi_log_get_level_name()
// TODO
//======================================================================
const char*
(twi_log_get_level_name)(
		const struct twi_log* log,
		uint8_t level_id
);

//======================================================================
// decl twi_log_get_level_abbrev()
// TODO
//======================================================================
const char*
(twi_log_get_level_abbrev)(
		const struct twi_log* log,
		uint8_t level_id
);

//======================================================================
// decl twi_log_get_level_codes()
// TODO
//======================================================================
const char*
(twi_log_get_level_codes)(
		const struct twi_log* log,
		uint8_t level_id
);

//=======================================================================
// decl twi_log_set_implicit_path_prefix()
//=======================================================================
void
(twi_log_set_implicit_path_prefix)(
		struct twi_log* log,
		const char* prefix
);

//======================================================================
// decl twi_log_write()
// TODO: Revise
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
#define twi_log_write(logptr, lvl, ...) \
	twi_log_write(logptr, __FILE__, __LINE__, lvl, __VA_ARGS__);
void
(twi_log_write)(
		struct twi_log* log,
		const char* fp,
		long lineno,
		uint8_t lvl,
		const char* msg,
		...
);

#endif // TWI_LOG_H
