//======================================================================
//======================================================================
// twi/std/log.c - twi_log source file
// Written by Serenity Twilight
//======================================================================
//======================================================================
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <twi/std/assert.h>
#include <twi/std/log.h>
#include <twi/std/status.h>

//======================================================================
//======================================================================
// Macros
//======================================================================
//======================================================================
// The following are macros that define argument assertions which are
// used at multiple locations throughout this file.
#define ASSERT_LT_u8(lhs, rhs) \
	twi_assertf(lhs < rhs, \
			"Value of argument `" #lhs "` must be less than the value of " \
			"argument `" #rhs "` (" #lhs " = %" PRIu8 ", " #rhs " = %" PRIu8 ").", \
			lhs, rhs)

// Basic debugging reporting through stdout.
#if TWI_LOG_DEBUG
#	define DEBUGMSG(...) printf(__VA_ARGS__);
#else
# define DEBUGMSG(...) ((void)0);
#endif

//======================================================================
//======================================================================
// Type definitions
//======================================================================
//======================================================================

//======================================================================
// decl enum twi_log_flags
// def enum twi_log_flags
//
// Defines flag bits for the twi_log.flags variable.
//----------------------------------------------------------------------
// * TWI_LOG_FLAG_EPOCH:
// Set if the epoch was successfully set at the creation of the log, or
// cleared otherwise.
//
// * TWI_LOG_FLAG_RECALC_LEVELS:
// Set if the cumulative level mask of a level is desynchronized from
// the individual level masks of the streams it owns, and therefore
// must be recalculated with a potentially non-trivial operation.
//======================================================================
enum twi_log_flags {
	TWI_LOG_FLAG_EPOCH = 0x01,
	TWI_LOG_FLAG_RECALC_LEVELS = 0x02
};

//======================================================================
// decl struct twi_log_stream
// def struct twi_log_stream
//
// Internal structure for management of log output streams.
//----------------------------------------------------------------------
// Members:
// * handle: References the output stream owned by this object.
//   Equal to NULL if this stream object does not own an output stream.
// * level_mask: Bitmask which identifies the user-defined logging
//   levels that will be able to write messages to the output stream
//   referenced by `handle`.
//======================================================================
struct twi_log_stream {
	FILE* handle;
	uint32_t level_mask;
};

#define DUMP_TWI_LOG_STREAM(stream) \
	DEBUGMSG("{handle = %p, level_mask = %" PRIu32 "}", (stream).handle, (stream).level_mask)

//======================================================================
// decl struct twi_log_level
// def struct twi_log_level
//
// Information describing a logging level.
// All members of this struct point to memory managed by the log's
// user, and therefore cannot be modified.
//
// See the description above the declaration of `twi_log_define_level()`
// for more details regarding the restrictions of these struct members.
//----------------------------------------------------------------------
// Members:
// * name:
// The human-readable name which designates a logging level.
//
// * abbrev:
// The shorthand abbreviation which designates a logging level.
//
// * codes:
// The character codes which may be used to enable a logging level.
//======================================================================
struct twi_log_level {
	const char* name;
	const char* abbrev;
	const char* codes;
};

#define DUMP_TWI_LOG_LEVEL(lvl) { \
	const char* name,* nquote,* abbrev,* aquote,* codes,* cquote; \
	if ((lvl).name != NULL) { \
		name = (lvl).name; \
		nquote = "\""; \
	} else { \
		name = "NULL"; \
		nquote = ""; \
	} \
	if ((lvl).abbrev != NULL) { \
		abbrev = (lvl).abbrev; \
		aquote = "\""; \
	} else { \
		abbrev = "NULL"; \
		aquote = ""; \
	} \
	if ((lvl).codes != NULL) { \
		codes = (lvl).codes; \
		cquote = "\""; \
	} else { \
		codes = "NULL"; \
		cquote = ""; \
	} \
	DEBUGMSG("{name = %s%s%s (%p), abbrev = %s%s%s (%p), codes = %s%s%s (%p)}", \
			nquote, name, nquote, name, \
			aquote, abbrev, aquote, abbrev, \
			cquote, codes, cquote, codes \
	); \
}

//======================================================================
// def struct twi_log
//
// The primary log management object.
//----------------------------------------------------------------------
// Members:
// * streams:
// Pointer to an array of size `num_streams` containing all of the
// output streams that a log is capable of maintaining simultaneously.
//
// * levels:
// Pointer to an array of size `num_levels` containing all of the
// user-defined logging levels that this log is capable of maintaining
// simultaneously.
//
// * implicit_path_prefix:
// A filepath prefix which, if matched with the start of a filepath
// used in a logging message, is truncated from the start of the
// filepath printed in the log message.
//
// * epoch:
// The timestamp of when a particular log instance is created.
//
// * cumulative_level_mask:
// A bitwise combination of the level_masks of all of a log's open
// streams.
//
// * num_streams:
// The maximum number of streams that a log can manage at once.
//
// * num_levels:
// The maximum number of user-defined logging levels that a log can
// manage at once.
//
// * flags:
// Internal flags used for tracking log state.
// See the definition of `enum twi_log_flags` for more details.
//======================================================================
struct twi_log {
	struct twi_log_stream* streams;
	struct twi_log_level* levels;
	const char* implicit_path_prefix;
	struct timespec epoch;
	uint32_t cumulative_level_mask;
	uint8_t num_streams;
	uint8_t num_levels;
	uint8_t flags;
};

#define DUMP_TWI_LOG(log) { \
	DEBUGMSG("{streams = {"); \
	for (uint_fast8_t s = 0; s < (log).num_streams; ++s) { \
		DUMP_TWI_LOG_STREAM((log).streams[s]); \
		if (s + 1 < (log).num_streams) \
			DEBUGMSG(","); \
	} \
	DEBUGMSG("}, levels = "); \
	for (uint_fast8_t l = 0; l < (log).num_levels; ++l) { \
		DUMP_TWI_LOG_LEVEL((log).levels[l]); \
		if (l + 1 < (log).num_levels) \
			DEBUGMSG(","); \
	} \
	const char* prefix,* pquote; \
	if ((log).implicit_path_prefix != NULL) { \
		prefix = (log).implicit_path_prefix; \
		pquote = "\""; \
	} else { \
		prefix = "NULL"; \
		pquote = ""; \
	} \
	DEBUGMSG("}, implicit_path_prefix = %s%s%s (%p), " \
			"epoch = DUMP TO-BE-IMPLEMENTED, " \
			"cumulative_level_mask = %" PRIu32 ", " \
			"num_streams = %" PRIu8 ", num_levels = %" PRIu8 ", flags = %" PRIu8 "}", \
			pquote, prefix, pquote, prefix, (log).cumulative_level_mask, \
			(log).num_streams, (log).num_levels, (log).flags \
	); \
}

//======================================================================
// def twi_log_errno
//======================================================================
_Thread_local enum twi_status twi_log_errno;

//======================================================================
//======================================================================
// Forward declarations of internal functions.
//======================================================================
//======================================================================
static uint32_t calculate_level_mask(uint8_t, struct twi_log_level[], const char*);
static int prepend_log_info(size_t, char[], const struct twi_log*, const char*, long, uint8_t);
static void report_stdio_failure(const char*, int);
static const char* trim_prefix(const char*, const char*);

//======================================================================
//======================================================================
// External functions
//======================================================================
//======================================================================

//=======================================================================
// def twi_log_create()
//=======================================================================
struct twi_log*
(twi_log_create)(uint8_t num_streams, uint8_t num_levels) {
	// Validate arguments.
	twi_assertf(num_streams > 0, "Argument `num_streams` cannot be 0.");
	twi_assertf(num_levels > 0, "Argument `num_levels` cannot be 0.");
	twi_assertf(num_levels <= 32, "Argument `num_levels` cannot exceed 32 (num_levels = %" PRIu8 ").", num_levels);

	// Measure number of bytes required to fit log.
	// The number of bytes allocated should account for alignment.
	size_t required_bytes = sizeof(struct twi_log);
	size_t streams_padding = _Alignof(struct twi_log_stream) - (required_bytes % _Alignof(struct twi_log_stream));
	if (streams_padding == _Alignof(struct twi_log_stream))
		streams_padding = 0; // Already aligned. Do not pad.
	required_bytes += streams_padding + (sizeof(struct twi_log_stream) * num_streams);
	size_t levels_padding = _Alignof(struct twi_log_level) - (required_bytes % _Alignof(struct twi_log_level));
	if (levels_padding == _Alignof(struct twi_log_level))
		levels_padding = 0; // Already aligned. Do not pad.
	required_bytes += levels_padding + (sizeof(struct twi_log_level) * num_levels);

	// Allocate.
	errno = 0;
	struct twi_log* log = malloc(required_bytes);
	if (log == NULL) {
		twi_log_errno = TWI_STATUS_NOMEM;
		return NULL;
	}

	// Initialize.
	log->streams = (struct twi_log_stream*)(((uint8_t*)log) + sizeof(struct twi_log) + streams_padding);
	log->levels = (struct twi_log_level*)(((uint8_t*)(log->streams)) + (sizeof(struct twi_log_stream) * num_streams) + levels_padding);
	log->implicit_path_prefix = NULL;
	log->cumulative_level_mask = 0;
	log->num_streams = num_streams;
	log->num_levels = num_levels;
	if (timespec_get(&(log->epoch), TIME_UTC) == TIME_UTC)
		log->flags = TWI_LOG_FLAG_EPOCH;
	else
		log->flags = 0;

	// Set streams and levels into default states that identifies them as empty.
	for (uint_fast8_t i = 0; i < num_streams; ++i) {
		log->streams[i].handle = NULL;
		log->streams[i].level_mask = 0;
	}
	for (uint_fast8_t i = 0; i < num_levels; ++i) {
		log->levels[i].name = NULL;
		log->levels[i].abbrev = NULL;
		log->levels[i].codes = NULL;
	}

	return log;
} // end twi_log_create()

//=======================================================================
// def twi_log_delete()
//=======================================================================
void
(twi_log_delete)(struct twi_log* log) {
	// Validate arguments.
	twi_assert_notnull(log);

	// Close all outstanding streams, then free the log object.
	// This is done manually rather than with twi_log_close_stream() to
	// avoid the overhead incurred in keeping the log object in a valid state
	// afterwards, which it unnecessary as the log object is being deleted.
	for (uint_fast16_t s = 0; s < log->num_streams; ++s) {
		if (log->streams[s].handle != NULL
		 && log->streams[s].handle != stdout
		 && log->streams[s].handle != stderr)
			fclose(log->streams[s].handle);
	}
	free(log);
} // end twi_log_delete()

//======================================================================
// def twi_log_open_stream()
//======================================================================
uint_fast8_t
(twi_log_open_stream)(
		struct twi_log* log,
		uint8_t stream_id,
		const char* path,
		uint_fast8_t append,
		const char* level_codes
) {
	// Validate arguments.
	twi_assert_notnull(log);
	ASSERT_LT_u8(stream_id, log->num_streams);
	twi_assert_notnull(path);

	// Access stream.
	FILE* new_handle;
	// If the stream path is "stdout", use existing stdout FILE*.
	// If the stream path is "stderr", use existing stderr FILE*.
	if (strncmp(path, "std", 3) == 0) {
		const char* stdtype = path + 3;
		if (strcmp(stdtype, "out") == 0)
			new_handle = stdout;
		else if (strcmp(stdtype, "err") == 0)
			new_handle = stderr;
		else // Path is not a std stream.
			goto open_file;
	} else {
open_file:
		// FIXME: Handle creation of new directories.
		errno = 0;
		new_handle = fopen(path, (append ? "a" : "w"));
		if (new_handle == NULL) {
			twi_log_errno = TWI_STATUS_NOSTREAM;
			return 1;
		}
	} // end if/else path references standard stream
	
	// (Re)initialize stream object.
	if (log->streams[stream_id].handle != NULL)
		twi_log_close_stream(log, stream_id);
	log->streams[stream_id].handle = new_handle;
	log->streams[stream_id].level_mask = calculate_level_mask(log->num_levels, log->levels, level_codes);
	log->cumulative_level_mask |= log->streams[stream_id].level_mask;

	return 0;
} // end twi_log_open_stream()

//======================================================================
// def twi_log_close_stream()
//======================================================================
void
(twi_log_close_stream)(
		struct twi_log* log,
		uint8_t stream_id
) {
	// Validate arguments.
	twi_assert_notnull(log);
	ASSERT_LT_u8(stream_id, log->num_streams);

	if (log->streams[stream_id].handle != NULL) {
		// DO NOT CLOSE STANDARD OUTPUT STREAMS!
		// Simply disassociate from them.
		if (log->streams[stream_id].handle != stdout
		 && log->streams[stream_id].handle != stderr)
			fclose(log->streams[stream_id].handle);
		log->streams[stream_id].handle = NULL;
		if (log->streams[stream_id].level_mask != 0) {
			log->streams[stream_id].level_mask = 0;
			log->flags |= TWI_LOG_FLAG_RECALC_LEVELS;
		}
	} // end if user-specified stream is open
} // end twi_log_close_stream()

//=======================================================================
// def twi_log_set_stream_level()
//=======================================================================
void
(twi_log_set_stream_level)(
		struct twi_log* log,
		uint8_t stream_id,
		const char* level_codes
) {
	// Validate arguments.
	twi_assert_notnull(log);
	ASSERT_LT_u8(stream_id, log->num_streams);

	// Forbid setting level mask if the stream is not open.
	if (log->streams[stream_id].handle == NULL)
		return;

	log->streams[stream_id].level_mask
		= calculate_level_mask(log->num_levels, log->levels, level_codes);
	log->flags |= TWI_LOG_FLAG_RECALC_LEVELS;
} // end twi_log_set_stream_level()

//=======================================================================
// def twi_log_define_level()
//=======================================================================
void
(twi_log_define_level)(
		struct twi_log* log,
		uint8_t level_id,
		const char* name,
		const char* abbrev,
		const char* codes
) {
	// Validate arguments.
	twi_assert_notnull(log);
	ASSERT_LT_u8(level_id, log->num_levels);

	// Overwrite.
	log->levels[level_id].name = name;
	log->levels[level_id].abbrev = abbrev;
	log->levels[level_id].codes = codes;
} // end twi_log_define_level()

//=======================================================================
// def twi_log_get_level_name()
//=======================================================================
const char*
(twi_log_get_level_name)(
		const struct twi_log* log,
		uint8_t level_id
) {
	twi_assert_notnull(log);
	ASSERT_LT_u8(level_id, log->num_levels);
	return log->levels[level_id].name;
} // end twi_log_get_level_name()

//=======================================================================
// def twi_log_get_level_abbrev()
//=======================================================================
const char*
(twi_log_get_level_abbrev)(
		const struct twi_log* log,
		uint8_t level_id
) {
	twi_assert_notnull(log);
	ASSERT_LT_u8(level_id, log->num_levels);
	return log->levels[level_id].abbrev;
} // end twi_log_get_level_abbrev()

//=======================================================================
// def twi_log_get_level_codes()
//=======================================================================
const char*
(twi_log_get_level_codes)(
		const struct twi_log* log,
		uint8_t level_id
) {
	twi_assert_notnull(log);
	ASSERT_LT_u8(level_id, log->num_levels);
	return log->levels[level_id].codes;
} // end twi_log_get_level_codes()

//=======================================================================
// def twi_log_set_implicit_path_prefix()
//=======================================================================
void
(twi_log_set_implicit_path_prefix)(
		struct twi_log* log,
		const char* prefix
) {
	twi_assert_notnull(log);
	log->implicit_path_prefix = prefix;
} // end twi_log_set_implicit_path_prefix()

//=======================================================================
// def twi_log_write()
//=======================================================================
void
(twi_log_write)(
		struct twi_log* log,
		const char* fp,
		long lineno,
		uint8_t lvl,
		const char* msg,
		...
) {
	// Validate arguments.
	twi_assert_notnull(log);
	ASSERT_LT_u8(lvl, log->num_levels);
#if TWI_LOG_DEBUG
	DEBUGMSG("TRACE ->twi_log_write(log = ");
	DUMP_TWI_LOG(*log);
	DEBUGMSG(", fp = DON'T CARE, lineno = %ld, lvl = %" PRIu8 ", msg = \"%s\" (%p))\n",
			lineno, lvl, msg, msg);
#endif

	// Recalculate cumulative_level_mask, if necessary.
	if (log->flags & TWI_LOG_FLAG_RECALC_LEVELS) {
		log->cumulative_level_mask = log->streams[0].level_mask;
		for (uint_fast8_t s = 1; s < log->num_streams; ++s)
			log->cumulative_level_mask |= log->streams[s].level_mask;
		log->flags &= ~TWI_LOG_FLAG_RECALC_LEVELS;
	} // end if the cumulative level mask must be recalculated

	// If no stream will accept this message,
	// then it can be discarded without processing.
	uint32_t lvl_mask = (1 << lvl);
	DEBUGMSG("lvl_mask = %" PRIu32 "\n", lvl_mask);
	if (!(log->cumulative_level_mask & lvl_mask))
		return;

	// Write the log message prefix (timestamp, level, and invocation location) to a buffer.
	char buf[2048]; // FIXME: Make more versatile against very large logging messages.
	int status = prepend_log_info(sizeof(buf), buf, log, fp, lineno, lvl);
	if (status < 0)
		return; // Formatting failure.

	int offset = status;
	// Write the user-provided log message to the buffer, after the prefix.
	va_list caller_args;
	va_start(caller_args, msg);
	errno = 0;
	status = vsnprintf(buf + offset, sizeof(buf) - offset, msg, caller_args);
	va_end(caller_args);
	if (status < 0) {
		report_stdio_failure("vsnprintf()", status);
		return;
	}

	offset += status;
	// Punctuate buffer with newline.
	if (offset+1 < sizeof(buf)) {
		buf[offset] = '\n';
		buf[offset+1] = 0;
	} else
		buf[offset-1] = '\n';
	

	// Copy the buffer contents to all open streams with `lvl` in their mask.
	for (uint_fast8_t s = 0; s < log->num_streams; ++s) {
		if (log->streams[s].level_mask & lvl_mask) {
			status = fputs(buf, log->streams[s].handle);
			if (status < 0)
				report_stdio_failure("fputs()", status);
		}
	} // end iteration over `log`'s streams.
} // end twi_log_write()

//======================================================================
//======================================================================
// Internal functions
//======================================================================
//======================================================================

//=======================================================================
// decl calculate_level_mask()
// def calculate_level_mask()
//
// Converts a collection of 1-character level codes into a bitmask
// with bits of equivalent levels found in `level_codes` set.
//-----------------------------------------------------------------------
// Parameters:
// * num_levels:
// The number of levels defined in `levels`. Behavior is undefined if
// this value is greater than the total number of levels in `levels`.
//
// * levels:
// The level definitions to compare against the contents of `level_codes`.
//
// * level_codes:
// The set of 1-character level codes to convert into a bitmask.
//-----------------------------------------------------------------------
// Returns:
// The bitmask equivalent of `level_codes` when adhering to the
// definitions contained within `levels`.
//=======================================================================
static uint32_t
(calculate_level_mask)(
		uint8_t num_levels,
		struct twi_log_level levels[num_levels],
		const char* level_codes
) {
	// Validate arguments.
	twi_assert_notnull(levels);
#if TWI_LOG_DEBUG
	DEBUGMSG("TRACE ->calculate_level_mask(num_levels = %" PRIu8 ", levels[] = {", num_levels);
	for (uint_fast8_t l = 0; l < num_levels; ++l) {
		DEBUGMSG("{name = \"%s\", abbrev = \"%s\", codes = \"%s\"}%s",
			levels[l].name, levels[l].abbrev, levels[l].codes, (l + 1 == num_levels ? "" : ","));
	}
	DEBUGMSG("}, level_codes = \"%s\" (%p))\n", level_codes, level_codes);
#endif

	if (level_codes == NULL)
		level_codes = "";

	// Calculate mask as the bitwise OR of all of the defined levels
	// which contain at least one character in their code which matches
	// at least one character in `level_codes`.
	uint32_t mask = 0;
	for (; *level_codes != 0; ++level_codes) {
		for (uint_fast8_t l = 0; l < num_levels; ++l) {
			for (const char* c = levels[l].codes; *c != 0; ++c) {
				if (*c == *level_codes) {
					mask |= (1 << l);
					break; // Level is now active. Not need to continue checking its codes.
				}
			} // end iteration over characters defined in each level's code
		} // end iteration over each level defined in `levels`
	} // end iteration over characters in `level_codes`
	DEBUGMSG("TRACE <-calculate_level_mask(%" PRIu32 ")\n", mask);
	return mask;
} // end calculate_level_mask()

//=======================================================================
// decl prepend_log_info()
// def prepend_log_info()
//
// Prepends log message metadata to `buf`.
//
// The standard log message format is as follows:
// [seconds.microseconds] (lvl):fp:lineno:(space)
//
// The metadata is suffixed with a space to simplify appending the
// formatted log message afterwards.
//-----------------------------------------------------------------------
// Parameters:
// * bufsz: The maximum number of bytes allowed to be written to `buf`,
//   including the terminating null byte.
// * buf: The character array of at least size `bufsz` to output the log
//   message metadata to. Behavior is undefined if `buf` is NULL or not at
//   least `bufsz` in length.
// * log: A twi_log returned by a call to `twi_log_create()` that has yet to
//   be destroyed by a call to `twi_log_delete()`. Providing any other
//   value results in undefined behavior.
// * fp: The filepath to the file from which the log is being invoked.
// * lineno: The line number of the file from where the log is being invoked.
// * lvl: The logging level denoting the classification of the log message.
//-----------------------------------------------------------------------
// Returns:
// The number of characters which would be written to buf if bufsz is
// not heeded, or a negative number on error.
//=======================================================================
static int
(prepend_log_info)(
		size_t bufsz, char buf[bufsz],
		const struct twi_log* log,
		const char* fp,
		long lineno,
		uint8_t lvl
) {
	// Validate arguments.
	twi_assert_notnull(buf);
	twi_assert_notnull(log);
	twi_assert_notnull(fp);
	twi_assertf(lineno > 0, "Argument `lineno` must be greater than 0 (`lineno` = %ld).", lineno);
	ASSERT_LT_u8(lvl, log->num_levels);

	// Trim path prefix, if necessary.
	if (log->implicit_path_prefix != NULL) {
		fp = trim_prefix(fp, log->implicit_path_prefix);
	}

	// Calculate timestamp.
	const char* timestamp_error = NULL;
	struct timespec now;
	if (log->flags & TWI_LOG_FLAG_EPOCH) {
		if (timespec_get(&now, TIME_UTC) != TIME_UTC)
			timestamp_error = "NO_NOW";
	} else
		timestamp_error = "NO_EPOCH";

	int status;
	if (timestamp_error == NULL) {
		// Print elapsed time to the microsecond.
		errno = 0;
		if ((status = snprintf(buf, bufsz,
				"[%lld.%06ld] (%s) %s:%ld: ", // [timestamp] (lvl) fp:lineno:
				((long long)difftime(now.tv_sec, log->epoch.tv_sec)),
				(now.tv_nsec - log->epoch.tv_nsec) / 1000,
				log->levels[lvl].abbrev, fp, lineno
		)) < 0) {
			report_stdio_failure("snprintf() with timestamp", status);
		}
	} else {
		// At least one of the two points in time necessary
		// to calculate the timestamp is unavailable.
		// Substitute timestamp with error message.
		errno = 0;
		if ((status = snprintf(buf, bufsz,
				"[%s] (%s) %s:%ld: ", // [timestamp_error] (lvl) fp:lineno:
				timestamp_error, log->levels[lvl].abbrev, fp, lineno
		)) < 0) {
			report_stdio_failure("snprintf() without timestamp", status);
		}
	} // end if/else timestamp aquisition succeeds or fails

	DEBUGMSG("TRACE <-prepend_log_info(%d)\n", status);
	return status;
} // end prepend_log_info()

//=======================================================================
// decl report_stdio_failure()
// def report_stdio_failure()
//
// Attempts to write an error message to the stderr stream reporting
// the failure of an stdio operation named by `identifier`, displaying
// its return status (stored in the `status` argument) and, if non-zero,
// the errno and its equivalent string representation.
//
// The C Standard does not require all stdio functions (the printf family
// of functions in particular) to set errno on failure, though some
// implementations do so of their own volition. It is recommended that
// errno be cleared (set to 0) before calling the stdio function you
// anticipate might fail. Doing so will ensure that the errno output
// by this function is related to the stdio failure being reported.
//
// Please note that this function utilizes stdio functions to report
// stdio failure, and as such may itself fail to report the error.
// In such case, there's no more that can be reasonably done to report
// the error, and as such this function does not return any status to
// indicate success or failure.
//-----------------------------------------------------------------------
// Parameters:
// * identifier: An adequate description of the cstdlib operation that
//   failed (often a function name is fine, but further disambiguation
//   may be necessary in some cases).
// * status: Integral status indicator returned by the offending stdio
//   function.
//=======================================================================
static void
(report_stdio_failure)(const char* identifier, int status) {
	if (errno) {
		fprintf(stderr, "twi_log internal error: %s returned %d\n"
				"\terrno = %d (\"%s\")\n",
				identifier, status, errno, strerror(errno));
	} else {
		fprintf(stderr, "twi_log internal error: %s returned %d\n"
				"errno = 0\n", identifier, status);
	}
} // end report_stdio_failure()

//=======================================================================
// decl trim_prefix()
// def trim_prefix()
//
// If the entire character array (excluding terminating null byte)
// pointed to by `prefix` matches the start of the character array
// pointed to by `str`, this function returns a character pointer
// to the first character of `str` which follows the final (non-null)
// character those of `prefix`, effectively trimming the matching prefix.
//
// If the contents of `prefix` and the start of `str` do not match
// exactly, then `str` is returned.
//
// Under no circumstances are any strings modified by this function.
// If `str` points to the start of dynamically allocated memory,
// ensure that the original pointer is used for deallocation.
//-----------------------------------------------------------------------
// Parameters:
// * str: Pointer to a null-terminated character array.
// * prefix: Pointer to a null-terminated character array representing
//   the prefix to trim from the start of `str`, if present.
//-----------------------------------------------------------------------
// Returns:
// (str + strlen(prefix)) if the start of the character array
// pointed to by `str` matches the entire character array
// (not including its terminating null byte) pointed to by `prefix`.
// If not, returns `str`.
//=======================================================================
static const char*
(trim_prefix)(
		const char* str,
		const char* prefix
) {
	// Validate arguments.
	twi_assert_notnull(str);
	twi_assert_notnull(prefix);

	// Compare `str` to `prefix`.
	// If the strings differ before the end of `prefix`,
	// return a pointer to the beginning of `str`.
	// Otherwise, return a pointer to the first character of
	// `str` after the prefix which matches `prefix`.
	size_t c;
	for (c = 0; prefix[c]; ++c)
		if (prefix[c] != str[c]) return str;
	return str + c;
} // end trim_prefix()

