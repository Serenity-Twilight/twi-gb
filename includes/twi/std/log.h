// TODO: File description. Probably need one for source file, too.
#ifndef TWI_LOG_H
#define TWI_LOG_H

#include <stdint.h>
#include <stdio.h>
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
//
// Arguments:
// * num_streams:
// The number of streams to allocate space for within this twi_log.
// Up to 255 streams can be managed by a single twi_log.
// It is undefined behavior to request 0 streams.
//
// * num_levels:
// The number of logging levels to allocate space for within this twi_log.
// Up to 32 logging levels can be defined for a single twi_log.
// It is undefined behavior to request 0 levels, or greater than 32 levels.
//
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
//
// Arguments:
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
//
// Arguments:
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
//
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
//
// Arguments:
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
//
// Arguments:
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
// TODO
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
		const char*, long, // Ignore these. Internal variables.
		uint8_t lvl,
		const char* msg,
		...
);

#endif // TWI_LOG_H
