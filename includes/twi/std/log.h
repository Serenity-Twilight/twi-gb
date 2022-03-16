//======================================================================
//======================================================================
// includes/twi/std/log.h
//
// A general purpose logging utility with the following features:
// * Management of up to 32 nonlinear user-defined logging levels,
//   which can be used to selectively filter out occurences of logging
//   based on debugging and performance needs.
// * Management of up to 255 independent output streams, each with its
//   own logging mask, allowing messages of a particular level to be
//   written to some streams without writing to others.
// * Log-wide filepath culling to reduce obvious, implicit components
//   from a filepath being written to a log output stream.
//   See the description of `twi_log_set_implicit_path_prefix()` for
//   more details.
// * Automation of printing useful metadata along with logging messages,
//   including time-since-initialization, logging level, filepath, and
//   line number.
// * Support for variable substitution in logging messages alike to
//   the printf family of functions.
//
// Written by Serenity Twilight.
//======================================================================
//======================================================================
#ifndef TWI_LOG_H
#define TWI_LOG_H

#include <stdint.h>
#include <twi/std/status.h>

//======================================================================
// def twi_log_errno
//
// Describes the specific reason for failure of the most recent
// twi_log operation which reported failure on this thread.
//======================================================================
extern _Thread_local enum twi_status twi_log_errno;

//======================================================================
// decl struct twi_log
//
// An opaque logging structure which represents the inteface between
// the program utilizing the log and the underlying output streams
// which accept logging input.
//
// All manipulation of a twi_log object available to the user is
// described in this file. Refer to the file's description for an
// overview of twi_log features, and individual function descriptions
// for a more detailed explanation of specific features.
//======================================================================
struct twi_log;

//======================================================================
// decl twi_log_create()
//
// Allocates memory for a new twi_log object and initializes it to a
// default state.
//
// Twi_log objects require that the maximum number of usable streams
// and levels be defined at the time of creation. These counts cannot
// be changed later without destroying the twi_log object and creating
// a new one. Not all requested streams/levels must be utilized
// (it is valid to open fewer streams and/or define fewer levels than
// the counts requested) and streams/levels can be repurposed and
// redefined as necessary through the twi_log's lifetime.
//-----------------------------------------------------------------------
// Parameters:
// * num_streams:
// The number of streams to allocate space for within this twi_log.
// Up to 255 streams can be managed by a single twi_log.
// It is undefined behavior to request 0 streams.
//
// * num_levels:
// The number of logging levels to allocate space for within this twi_log.
// Up to 32 logging levels can be defined for a single twi_log.
// It is undefined behavior to request 0 levels, or greater than 32 levels.
//-----------------------------------------------------------------------
// Returns:
// An opaque pointer to the newly allocated and initialized twi_log
// object, or NULL if the function was unable to allocate sufficient
// memory. If this function returns NULL, `twi_log_errno` will be set
// to TWI_STATUS_NOMEM.
//======================================================================
struct twi_log*
(twi_log_create)(uint8_t num_streams, uint8_t num_levels);

//======================================================================
// decl twi_log_delete()
//
// Deinitializes and deallocates a twi_log object.
//
// If any streams are still open within the twi_log object when this
// function is called, they will be closed as if twi_log_close_stream
// had been called for each individual stream.
//-----------------------------------------------------------------------
// Parameters:
// * log:
// A pointer to a previously initialized twi_log object.
// It is undefined behavior for `log` to point to NULL, to point to
// any region of memory that was not returned by `twi_log_create`,
// or to point to a twi_log which has already been deleted by a previous
// call to twi_log_delete() (assuming the memory has not been repurposed
// in the interim by another call to twi_log_create()).
//======================================================================
void
(twi_log_delete)(struct twi_log* log);

//======================================================================
// decl twi_log_open_stream()
//
// Opens a new output stream to be managed by the twi_log object.
//
// Streams written to by twi_log can either be standard output streams
// (stdout/stderr) or files identified by a path. In order to write to
// standard output streams, the `path` argument should point to a
// null-terminated character array of either "stdout" or "stderr".
// All other paths will be assumed to refer to files.
//
// If the `stream_id` argument identifies a region within the twi_log
// which is already managing another open stream, the previous stream
// will be closed as if by call to twi_log_close_stream() first.
// See the description of twi_Log_close_stream() for specific details.
//
// Please note that this function does not check against currently open
// streams to ensure that all streams are unique. Therefore, it is
// possible to open multiple streams in twi_log which refer to the
// same destination stream. In such cases, the order to writing which
// occurs is implementation defined, and the final output can be
// unpredictable.
//-----------------------------------------------------------------------
// Parameters:
// * log:
// Pointer to a twi_log object.
// It is undefined behavior for `log` to point to NULL, a region of
// memory not returned by a call to twi_log_create(), or a twi_log
// which has already been deleted by a call to twi_log_delete().
//
// * stream_id:
// An integral identifier indicating the internal region of memory to
// contain the stream-to-be-opened. Must be a value from 0 to
// (the maximum number of streams - 1). It is undefined behavior to
// supply a `stream_id` outside of these bounds.
//
// * path:
// A pointer to a null-terminated character array identifying the
// stream to be opened. If `path` is equal to "stdout" or "stderr",
// the newly opened stream will attach to the existing stdout or stderr
// standard output stream. Otherwise, a new stream will be opened
// at the file identified by `path`.
//
// * append:
// Whether or not to clear the previous file referred to by `path`
// when opening the stream, or begin appending to the end of its
// existing contents. A value of 0 will wipe out existing file contents,
// whereas any non-zero value will append to it.
// Note that this has no effect when `path` is equal to "stdout" or "stderr".
//
// * level_codes:
// A sequence of character codes which define what logging levels will
// be written to this stream after it has been opened.
// For each individual character of `level_codes` that matches any
// one individual character defined within a logging level's codes,
// that logging level will be enabled.
// For example, if `level_codes` = "a", all logging levels whose `codes`
// variable contains at least one 'a' will be made active.
// If `level_codes` = "abc", all logging levels whose `codes` variable
// contains at least one 'a', 'b', or 'c' will be made active.
//-----------------------------------------------------------------------
// Returns:
// 0 if the new stream was opened successfully, or non-zero otherwise.
// If this function returns non-zero, twi_log_errno is set to
// TWI_STATUS_NOSTREAM, and the twi_log object is not modified.
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
//
// Closes or disassociates from a currently open stream.
//
// If the stream referenced by `stream_id` is a standard output stream
// (stdout or stderr), then it is disassociated from without further
// modification. The standard output stream will remain open for the
// greater process. Otherwise, the stream will be closed.
//
// If the region of memory referenced by `stream_id` is not currently
// managing any open stream, then this function does nothing.
//-----------------------------------------------------------------------
// Parameters:
// * log:
// Pointer to a twi_log object.
// It is undefined behavior for `log` to point to NULL, a region of
// memory not returned by a call to twi_log_create(), or a twi_log
// which has already been deleted by a call to twi_log_delete().
//
// * stream_id:
// An integral reference to one of the streams managed by the twi_log.
// Must be greater than or equal to 0 and less than the maximum number
// of streams managed by the twi_log, set by the call to twi_log_create().
// Providing a value outside of this range is undefined behavior.
//======================================================================
void
(twi_log_close_stream)(
		struct twi_log* log,
		uint8_t stream_id
);

//======================================================================
// decl twi_log_set_stream_level()
//
// Redefines the logging levels accepted by a particular stream.
//-----------------------------------------------------------------------
// Parameters:
// * log:
// Pointer to a twi_log object.
// It is undefined behavior for `log` to point to NULL, a region of
// memory not returned by a call to twi_log_create(), or a twi_log
// which has already been deleted by a call to twi_log_delete().
//
// * stream_id:
// An integral reference to one of the streams managed by the twi_log.
// Must be greater than or equal to 0 and less than the maximum number
// of streams managed by the twi_log, set by the call to twi_log_create().
// Providing a value outside of this range is undefined behavior.
//
// * level_codes:
// A sequence of character codes which define what logging levels will
// be written to this stream after the call to this function has
// returned.
// For each individual character of `level_codes` that matches any
// one individual character defined within a logging level's codes,
// that logging level will be enabled.
// For example, if `level_codes` = "a", all logging levels whose `codes`
// variable contains at least one 'a' will be made active.
// If `level_codes` = "abc", all logging levels whose `codes` variable
// contains at least one 'a', 'b', or 'c' will be made active.
//======================================================================
void
(twi_log_set_stream_level)(
		struct twi_log* log,
		uint8_t stream_id,
		const char* level_codes
);

//======================================================================
// decl twi_log_define_level()
//
// Defines the name, abbreviation, and mask codes used for one of
// the logging levels allocated at the twi_log's creation.
//
// A level's...
// * Name is its full, human-readable name.
// * Abbreviation is a shorthand indicator for a level. Internally, this
//   is used to indicate a log message's level in logging output.
// * Codes define which characters can be provided as level_codes when
//   either opening a new stream or setting the level of an existing
//   stream. If one or more of the characters used in level_codes when
//   setting a stream's level matches one or more of the characters
//   in a logging level's codes, that logging level is enabled for that
//   stream. One character can exist in any number of logging level codes.
//
// IMPORTANT:
// The twi_log takes ownership of memory pointed to by the null-terminated
// character arrays `name`, `abbrev`, and `codes` from the memory address
// each pointer points to up to and including the null byte at the end
// of each array.
// Ownership is relinquished by the twi_log when either
// twi_log_define_level() is called again for the same level in the same
// twi_log with different values or by calling twi_log_delete() on the
// twi_log.
// It is undefined behavior to change the contents of the null-terminated
// character arrays or for the null-terminated character arrays' lifetimes
// to end before ownership of them is relinquished by the twi_log.
//-----------------------------------------------------------------------
// Parameters:
// * log:
// Pointer to a twi_log object.
// It is undefined behavior for `log` to point to NULL, a region of
// memory not returned by a call to twi_log_create(), or a twi_log
// which has already been deleted by a call to twi_log_delete().
//
// * level_id:
// The level's integral identifier. Must be greater than or equal to 0
// and less the number of levels the twi_log was initiated with.
// Supplying a `level_id` outside of these bounds is undefined behavior.
//
// * name:
// The full, human-readable name to give to this logging level.
// Can be NULL, which makes the logging level nameless.
// See the IMPORTANT notice above for memory management information.
//
// * abbrev:
// The abbreviated name to give to this logging level.
// Can be NULL, which makes the logging level have no abbreviation.
// See the IMPORTANT notice above for memory management information.
//
// * codes:
// The codes that may be used to enable this logging level.
// Can be NULL, which makes the logging level unable to be enabled.
// See the IMPORTANT notice above for memory management information.
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
//
// Returns the name of the indicating logging level, or NULL if the
// indicated level is nameless.
//
//-----------------------------------------------------------------------
// Parameters:
// * log:
// Pointer to a twi_log object.
// It is undefined behavior for `log` to point to NULL, a region of
// memory not returned by a call to twi_log_create(), or a twi_log
// which has already been deleted by a call to twi_log_delete().
//
// * level_id:
// The level's integral identifier. Must be greater than or equal to 0
// and less the number of levels the twi_log was initiated with.
// Supplying a `level_id` outside of these bounds is undefined behavior.
//-----------------------------------------------------------------------
// Returns:
// The name of the logging level indicated.
// It is undefined behavior to either modify or relinquish the memory
// pointed to before the twi_log relinquishes ownership of it.
//======================================================================
const char*
(twi_log_get_level_name)(
		const struct twi_log* log,
		uint8_t level_id
);

//======================================================================
// decl twi_log_get_level_abbrev()
//
// Returns the abbreviation of the indicating logging level, or NULL if
// the indicated level has no abbreviation.
//
//-----------------------------------------------------------------------
// Parameters:
// * log:
// Pointer to a twi_log object.
// It is undefined behavior for `log` to point to NULL, a region of
// memory not returned by a call to twi_log_create(), or a twi_log
// which has already been deleted by a call to twi_log_delete().
//
// * level_id:
// The level's integral identifier. Must be greater than or equal to 0
// and less the number of levels the twi_log was initiated with.
// Supplying a `level_id` outside of these bounds is undefined behavior.
//
// Returns:
// The abbreviation of the logging level indicated.
// It is undefined behavior to either modify or relinquish the memory
// pointed to before the twi_log relinquishes ownership of it.
//======================================================================
const char*
(twi_log_get_level_abbrev)(
		const struct twi_log* log,
		uint8_t level_id
);

//======================================================================
// decl twi_log_get_level_codes()
// 
// Returns the enabling codes of the indicating logging level, or NULL if
// it does not have any defined enabling codes.
//
//-----------------------------------------------------------------------
// Parameters:
// * log:
// Pointer to a twi_log object.
// It is undefined behavior for `log` to point to NULL, a region of
// memory not returned by a call to twi_log_create(), or a twi_log
// which has already been deleted by a call to twi_log_delete().
//
// * level_id:
// The level's integral identifier. Must be greater than or equal to 0
// and less the number of levels the twi_log was initiated with.
// Supplying a `level_id` outside of these bounds is undefined behavior.
//
// Returns:
// The enabling codes of the logging level indicated.
// It is undefined behavior to either modify or relinquish the memory
// pointed to before the twi_log relinquishes ownership of it.
//======================================================================
const char*
(twi_log_get_level_codes)(
		const struct twi_log* log,
		uint8_t level_id
);

//=======================================================================
// decl twi_log_set_implicit_path_prefix()
//
// Sets the implicit path prefix for the twi_log, which is used for
// culling unimportant filepath information written to log files,
// thereby reducing the width of log message lines.
//
// The implicit path prefix is matched against the filepath string
// indicating the file a log message was written from. If the implicit
// path prefix matches the filepath exactly up to but not including the
// implicit path prefix's terminating null byte, the matching portion
// will not be written to the log file. Instead, the filepath will appear
// as everything AFTER the matched prefix.
//
// As an example, imagine that a log message is being written from a
// file at "src/main.c".
//
// * Without an implicit path prefix, the filepath will appear as "src/main.c".
// * With an implicit path prefix of "src/", the filepath will appear as "main.c".
// * With an implicit path prefix of "sr", the filepath will appear as "c/main.c".
// * With an implicit path prefix of "ma", the filepath will appear as "src/main.c".
// * With an implicit path prefix of "src/main.c/dummy", the filepath will appear as "src/main.c".
//
// Note that the implicit path prefix cannot be set for individual
// streams. Changes to the implicit path prefix affect all streams
// owned by a twi_log.
//
// IMPORTANT:
// The twi_log pointed to by `log` will assume ownership of the memory
// pointed to by `prefix` up to and including the terminating null byte.
// It is undefined behavior to modify the memory at `prefix` or for the
// memory pointed to by `prefix` to be freed/go out of scope while the
// twi_log assumes ownership.
// The twi_log will relinquish ownership of the memory pointed to by
// `prefix` when either twi_log_set_implicit_path_prefix() is called on
// the twi_log again, or twi_log_delete() is called on the twi_log.
//-----------------------------------------------------------------------
// Parameters:
// * log:
// Pointer to a twi_log object.
// It is undefined behavior for `log` to point to NULL, a region of
// memory not returned by a call to twi_log_create(), or a twi_log
// which has already been deleted by a call to twi_log_delete().
//
// * prefix:
// The prefix to set for this twi_log.
// If NULL, then the truncation of filepath prefixes is disabled.
//=======================================================================
void
(twi_log_set_implicit_path_prefix)(
		struct twi_log* log,
		const char* prefix
);

//======================================================================
// decl twi_log_write()
//
// General-purpose logging function for twi_log.
//
// Writes the contents of `msg` and all variadic arguments, formatted
// as if with the printf family of functions, to all streams currently
// open in `log` with the logging level `lvl` enabled.
//
// The output written to each stream which meets the aforementioned
// conditions is formatted as follows:
// 1. A timestamp, in the format of "[seconds.microseconds]", since the
//    initialization of `log`.
// 2. An predefined abbreviation for `lvl`, wrapped in parantheses.
// 3. The file and line in which this function was invoked by the caller.
// 4. The contents of `msg` with all substitutions performed using the
//    following variadic argument list, as if by the printf family of
//    functions.
// 5. A newline.
//
// If this function encounters an error whilst running, it will print
// an error message to stderr, without indicating to the running
// process that the error occurred. This is to simplify logging
// call sites, preventing callers from being concerned with the
// failures of the log.
//
//----------------------------------------------------------------------
// Parameters:
// * log:
// A pointer to a twi_log returned by twi_log_create() that has not
// yet been destroyed by twi_log_delete(). Any other value shall result
// in undefined behavior.
//
// * lvl:
// One of the logging levels defined within `log` which defines the
// priority level of the message. Must be greater than or equal to 0
// and less than that number of levels that `log` was created to contain.
// Any other value is undefined behavior.
//
// * msg:
// The message to write to all output streams managed by `log` which
// accept messages of level `lvl`. The message may optionally contain
// substitution patterns alike to the printf family of functions to
// perform substitution of variable values into the output. It is
// undefined behavior if `msg` points to NULL or a character array not
// terminated with the null byte.
//
// * ...:
// Variadic argument list, matching the total number and type
// sequenuence of substitution pattern present in `msg`.
//======================================================================
#define twi_log_write(logptr, lvl, ...) \
	twi_log_write(logptr, __FILE__, __LINE__, lvl, __VA_ARGS__)
void
(twi_log_write)(
		struct twi_log* log,
		const char*, long, // Ignore these. Internal variables.
		uint8_t lvl,
		const char* msg,
		...
);

#endif // TWI_LOG_H
