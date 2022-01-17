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

#include <twi/std/assertf.h>
#include <twi/std/log.h>
#include <twi/std/status.h>

//======================================================================
//======================================================================
// Macros
//======================================================================
//======================================================================
// The following are macros that define argument assertions which are
// used at multiple locations throughout this file.
#define ASSERT_NOTNULL(ptr) \
	twi_assertf(ptr != NULL, "Argument `" #ptr "` cannot be NULL.")
#define ASSERT_LT_u8(lhs, rhs) \
	twi_assertf(lhs < rhs, \
			"Value of argument `" #lhs "` must be less than the value of " \
			"argument `" #rhs "` (" #lhs " = %" PRIu8 ", " #rhs " = %" PRIu8 ").", \
			lhs, rhs)

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
// TODO: Improve and complete documenation.
//======================================================================
enum twi_log_flags {
	TWI_LOG_FLAG_EPOCH = 0x01,
	TWI_LOG_FLAG_HAS_STREAM = 0x02,
	TWI_LOG_FLAG_RECALC_LEVELS = 0x04,
	TWI_LOG_FLAG_RECALC_HAS_STREAM = 0x08
};

//======================================================================
// decl struct twi_log_stream
// def struct twi_log_stream
//
// Internal structure for management of log output streams.
//
// Members:
// * handle: References the output stream owned by this object.
//   Equal to NULL if this stream object does not own an output stream.
// * active_levels: Bitmask which identifies the user-defined logging
//   levels that will be able to write messages to the output stream
//   referenced by `handle`.
//======================================================================
struct twi_log_stream {
	FILE* handle;
	uint32_t active_levels;
};

//======================================================================
// decl struct twi_log_level
// def struct twi_log_level
// TODO
//======================================================================
struct twi_log_level {
	const char* name;
	const char* abbrev;
	const char* codes;
};

//======================================================================
// def struct twi_log_level
// TODO
//======================================================================
struct twi_log {
	struct twi_log_stream* streams;
	struct twi_log_level* levels;
	const char* implicit_path_prefix;
	struct timespec epoch;
	uint32_t all_active_levels;
	uint8_t num_streams;
	uint8_t num_levels;
	uint8_t flags;
};

//======================================================================
// def twi_log_errno
// TODO
//======================================================================
_Thread_local int twi_log_errno;

//======================================================================
//======================================================================
// Forward declarations of internal functions.
//======================================================================
//======================================================================
static const char*
log_level_strshort(enum twi_log_level);
static size_t
max_resolved_pathlen(const char*);
static int
prepend_log_info(
		size_t, char[],
		struct twi_log*,
		enum twi_log_level,
		const char*,
		long,
		const char*
);
static void
report_stdio_failure(const char*, int);
static FILE*
resolve_path(const char*, time_t, uint8_t);
static int_fast8_t
rotate_indexed_logs(const char*, const char*, uint8_t);
static const char*
trim_prefix(const char*, const char*);

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
	twi_assertf(num_levels <= 32, "Argument `num_levels` cannot exceed 32 (num_levels = %" PRIu8 ").", num_levels);
	// TODO: Deal with num_streams and/or num_levels equaling 0.

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
	log->streams = ((uint8_t*)log) + sizeof(struct twi_log) + streams_padding;
	log->levels = ((uint8_t*)(log->streams)) + (sizeof(struct twi_log_stream) * num_streams) + levels_padding;
	log->implicit_path_prefix = NULL;
	log->all_active_levels = 0;
	log->num_streams = num_streams;
	log->num_levels = num_levels;
	if (timespec_get(&(log->epoch), TIME_UTC) != TIME_UTC)
		log->flags = TWI_LOG_FLAG_EPOCH;
	else
		log->flags = 0;

	// Set streams and levels into default states that identifies them as empty.
	for (uint_fast8_t i = 0; i < num_streams; ++i) {
		log->streams[i].handle = NULL;
		log->streams[i].active_levels = 0;
	}
	for (uint_fast8_t i = 0; i < num_levels; ++i) {
		log->levels[i].name = NULL;
		log->levels[i].abbrev = NULL;
		log->levels[i].code = NULL;
	}

	return log;
} // end twi_log_create()

//=======================================================================
// def twi_log_delete()
//=======================================================================
void
(twi_log_delete)(struct twi_log* log) {
	// Validate arguments.
	ASSERT_NOTNULL(log);
	// TODO: Close all streams.
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
	ASSERT_NOTNULL(log);
	ASSERT_LT_u8(stream_id, log->num_streams);
	ASSERT_NOTNULL(path);

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
		// TODO: Handle creation of new directories.
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
	twi_log_set_stream_level(log, stream_id, level_codes);
	log->flags |= TWI_LOG_FLAG_HAS_STREAM;

	return 0;
} // end twi_log_open_stream()

//======================================================================
// def twi_log_stream_open_timestamped()
//======================================================================
//int_fast8_t
//twi_log_stream_open_timestamped(
//		struct twi_log* log,
//		const char* prefix,
//		const char* suffix
//) {
//	assertf(log != NULL, "Argument `log` points to NULL.");
//
//	// Retrieve current timestamp.
//	errno = 0;
//	time_t now = time(NULL);
//	if (now == (time_t)(-1)) {
//		twi_log_err_libc = "time";
//		return -1;
//	}
//
//	// Convert timestamp to calendar time.
//	errno = 0;
//	struct tm* gmt_now = gmtime(&now);
//	if (gmt_now == NULL) {
//		twi_log_err_libc = "gmtime";
//		return -1;
//	}
//
//	// Construct full pathname in the format of...
//	// {prefix}{timestamp}{suffix}
//	size_t remaining = TWI_LOG_PATHNAME_BUFSZ;
//	char pathname[TWI_LOG_PATHNAME_BUFSZ];
//	// Concatenate prefix.
//	if (prefix != NULL) {
//		size_t prefix_len = strlen(prefix);
//		if (prefix_len >= remaining)
//			return 1;
//		strcpy(pathname, prefix);
//		remaining -= prefix_len;
//	}
//
//	// Concatenate timestamp.
//	errno = 0;
//	int snprintf_status = snprintf(
//			(pathname + (TWI_LOG_PATHNAME_BUFSZ - remaining)),
//			remaining,
//			"%04d-%02d-%02d_%02d;%02d;%02d",
//			gmt_now->tm_year + 1900,
//			gmt_now->tm_mon + 1,
//			gmt_now->tm_mday,
//			gmt_now->tm_hour,
//			gmt_now->tm_min,
//			gmt_now->tm_sec
//	);
//
//	if (snprintf_status < 0) {
//		twi_log_err_libc = "snprintf";
//		return -1; // Internal error within snprintf().
//	} else if (snprintf_status >= remaining)
//		return 1; // Timestamp could not fit in remaining pathname buffer.
//	remaining -= snprintf_status;
//
//	// Concatenate suffix.
//	if (suffix != NULL) {
//		size_t suffix_len = strlen(suffix);
//		if (suffix_len >= remaining)
//			return 1;
//		strcpy(pathname + (TWI_LOG_PATHNAME_BUFSZ - remaining), suffix);
//	}
//
//	return twi_log_stream_open(log, pathname);
//} // end twi_log_stream_open_timestamped()
//
////======================================================================
//// def twi_log_stream_open_indexed()
////======================================================================
//int_fast8_t
//twi_log_stream_open_indexed(
//		struct twi_log* log,
//		const char* prefix,
//		const char* suffix,
//		uint8_t max_files
//) {
//	assertf(log != NULL, "Argument `log` points to NULL.");
//	assertf(max_files > 0, "Argument `max_files` must be greater than zero.");
//
//	int_fast8_t status = rotate_indexed_logs(prefix, suffix, max_files);
//	if (status != 0)
//		return status;
//
//	if (prefix == NULL)
//		prefix = "";
//	if (suffix == NULL)
//		suffix = "";
//	char pathname[TWI_LOG_PATHNAME_BUFSZ];
//	errno = 0;
//	int libc_status = snprintf(pathname, TWI_LOG_PATHNAME_BUFSZ,
//			"%s000%s", prefix, suffix);
//	if (libc_status < 0) {
//		twi_log_err_libc = "snprintf";
//		return -1;
//	}
//
//	return twi_log_stream_open(log, pathname);
//} // end twi_log_stream_open_indexed()

//======================================================================
// def twi_log_close_stream()
//======================================================================
void
(twi_log_close_stream)(
		struct twi_log* log,
		uint8_t stream_id
) {
	// Validate arguments.
	ASSERT_NOTNULL(log);
	ASSERT_LT_u8(stream_id, log->num_streams);

	if (log->streams[stream_id].handle != NULL) {
		// TODO: Check for fclose error?
		fclose(log->streams[stream_id].handle);
		log->streams[stream_id].handle = NULL;
		if (log->streams[stream_id].active_levels != 0) {
			log->streams[stream_id].active_levels = 0;
			log->flags |= TWI_LOG_FLAG_RECALC_LEVELS;
		}
		log->flags |= TWI_LOG_FLAG_RECALC_HAS_STREAM;
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
	ASSERT_NOTNULL(log);
	ASSERT_LT_u8(stream_id, log->num_streams);
	if (level_codes == NULL)
		level_codes = "";

	// Compare characters within the `level_codes` argument to
	// character codes of registered levels of `log`.
	// Every time a character match is found, add that level to the
	// stream's level mask.
	log->streams[stream_id].active_levels = 0;
	for (; *level_codes != 0; ++level_codes) {
		for (uint_fast8_t l = 0; l < log->num_levels; ++l) {
			for (const char* c = log->levels[l].codes; *c != 0; ++c) {
				if (*c == *level_codes) {
					log->streams[stream_id].active_levels |= (1 << l);
					break; // Level is active, no need to keeping checking its codes.
				}
			} // end iteration over codes of a level in `log`
		} // end iteration over levels in `log`
	} // end iteration over characters in `level_codes`
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
	ASSERT_NOTNULL(log);
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
		struct twi_log* log,
		uint8_t level_id
) {
	ASSERT_NOTNULL(log);
	ASSERT_LT_u8(level_id, log->num_levels);
	return log->levels[level_id].name;
} // end twi_log_get_level_name()

//=======================================================================
// def twi_log_get_level_abbrev()
//=======================================================================
const char*
(twi_log_get_level_abbrev)(
		struct twi_log* log,
		uint8_t level_id
) {
	ASSERT_NOTNULL(log);
	ASSERT_LT_u8(level_id, log->num_levels);
	return log->levels[level_id].abbrev;
} // end twi_log_get_level_abbrev()

//=======================================================================
// def twi_log_get_level_codes()
//=======================================================================
const char*
(twi_log_get_level_codes)(
		struct twi_log* log,
		uint8_t level_id
) {
	ASSERT_NOTNULL(log);
	ASSERT_LT_u8(level_id, log->num_levels);
	return log->levels[level_id].codes;
} // end twi_log_get_level_codes()

//=======================================================================
// def twi_log_write()
//=======================================================================
#undef twi_log_write
void
twi_log_write(
		struct twi_log* log,
		enum twi_log_level lvl,
		const char* fp,
		long lineno,
		const char* msg,
		...
) {
	assertf(log, "`log` points to NULL.");

	// If no output stream is open, do nothing.
	if (log->stream == NULL)
		return;

	char buf[BUFSIZ];
	int status = prepend_log_info(BUFSIZ, buf, log, lvl, fp, lineno, msg);
	if (status < 0) {
		return; // Formatting failure.
	}

	// Substitute caller-provided arguments into the caller's message string
	// and write to the output stream.
	va_list caller_args;
	va_start(caller_args, msg);
	errno = 0;
	status = vfprintf(log->stream, buf, caller_args);
	va_end(caller_args);
	if (status < 0) {
		report_stdio_failure("vfprintf()", status);
	}
} // end twi_log_write()

//======================================================================
//======================================================================
// Internal functions
//======================================================================
//======================================================================

//=======================================================================
// decl log_level_strshort()
// def log_level_strshort()
//
// Provides a short-hand abbreviated string equivalent to the provided
// twi_log_level fit for width-constrained logging output.
//
// Parameter:
// * level - A twi_log_level.
// Returns:
// A pointer to a statically allocated, null-terminated character
// array containing an abbreviated, printable representation of `level`.
//=======================================================================
static const char*
log_level_strshort(enum twi_log_level level) {
	assertf(level >= TWI_LOG_LEVEL_NONE && level <= TWI_LOG_LEVEL_TRACE,
			"`level` is not a defined twi_log_level enumeration (%d)", level);
	switch (level) {
		case TWI_LOG_LEVEL_NONE: return "00";
		case TWI_LOG_LEVEL_FATAL: return "FF";
		case TWI_LOG_LEVEL_ERROR: return "EE";
		case TWI_LOG_LEVEL_WARN: return "WW";
		case TWI_LOG_LEVEL_INFO: return "II";
		case TWI_LOG_LEVEL_DEBUG: return "DD";
		case TWI_LOG_LEVEL_TRACE: return "TT";
	}
} // end log_level_strshort()

//=======================================================================
// decl max_resolved_pathlen()
// def max_resolved_pathlen()
//
// Indicates the maximum number of characters that can appear within
// the provided path after all of its substitution symbols have been
// fully resolved.
//
// If `path` is equal to NULL, behavior is undefined.
//
// Parameters:
// * path:
// Pointer to a null-terminated character array describing the path
// to an outstream.
//
// Returns:
// The number of narrow-width characters the provided path can
// potentially have after all of its substitutions are resolved.
//=======================================================================
static size_t
max_resolved_pathlen(const char* path) {
	twi_assertf(path, "`path` cannot be NULL.");
	// Iterate through `path`'s character array.
	// Upon finding any valid substitution symbols, add the maximum
	// number of characters added to `increase`, or the minimum number
	// of characters removed to `decrease`. The final result will be:
	// length of `path` + increase - decrease
	size_t increase = 0;
	size_t decrease = 0;
	const char* pos;
	for (pos = path; pos; ++pos) {
		if (*pos == '%') {
			// Evaluate the character following the substition character.
			// If substitution increases the length of `path`, add 
			// max additional length to `increase`.
			// If susbtitution decreases the length of `path`, add max
			// additional length to `decrease`.
			// NOTE: The substitution-indicating characters will be removed.
			// The length of the substitution characters (2) should be subtracted from
			// the increase/decrease of overall path length.
			++pos;
			switch(*pos) {
				case '%': // Escape substitution and use a single percent sign: 1
					decrease += 1; // (1 - 2 = -1)
					break;
				case '@': // Timestamp. YYYY-MM-DDTHH:MM:SS: 19
					increase += 17; // (19 - 2 = 17)
					break;
				case '#': // Index. (000-255): 3
					increase += 1; // (3 - 2 = 1)
					break;
				case 0: // End of string. Terminate loop immediately.
					goto end;
			} // end evaluating character following substitution indicator
		} // end if character is substitution character
	} // end iteration through `path`

end:
	return (size_t)(pos - path) + increase - decrease;
} // end max_resolved_pathlen()

//=======================================================================
// decl path_subst()
// def path_subst()
//
// Performs substitutions indicated within `path`, and stores the
// fully resolved string in `dest`.
//
// See the description of twi_log_open_stream() for a detailed
// description of supported path substitutions.
//
// If `path` or `dest` are equal to NULL, behavior is undefined.
//
// Parameters:
// * dest:
// Pointer to the destination to which the contents of `path` are
// written after all substitutions are resolved.
// * path:
// Pointer to a null-terminated character array containing a path to
// which to open an outstream, which may or may not contain supported
// substitution instructions.
// * epoch:
// Implementation-defined time value that defines the time at which the
// twi_log was initialized.
// * max_index:
//=======================================================================


//=======================================================================
// decl prepend_log_info()
// def prepend_log_info()
//
// Prepends standard formatted log message information to the log
// message `msg`, and stores the result in `buf` up to a maximum of
// `bufsz` - 1 characters, followed by a null byte.
//
// The standard log message format is as follows:
// [timestamp] (lvl):fp:lineno: msg
//
// The timestamp format is defined as follows:
// [days:hours:minutes:seconds:microseconds]
// The number of days is excluded, along with the colon following it,
// if less than one day has passed since the initialization of `log`.
//
// The number of hours, minutes, and seconds are each zero-padded to
// be exactly 2 characters wide, and microseconds zero-padded to be
// exactly 6 characters wide, to improve readibility via consistent
// horizontal alignment of log messages. The day counter, if present,
// has no minimum or maximum width.
//
// `lvl` is formatted via log_level_strshort().
//
// The filepath pointed to by `fp` will have the implicit_fp,
// defined by `log`, trimmed off its front if present before writing
// it to `buf`.
//
// No substitutions are performed on the contents of `msg` when
// prepending standard log message information to it.
//
// Parameters:
// - bufsz: The maximum number of characters allowed to be written to
//   `buf`, including terminating null character.
// - buf: The character array of at least size `bufsz` to output the
//   formatted log message information and unformatted message to.
// - log: Pointer to an initialized twi_log, whose epoch is used to
//   calculate the timestamp used in the output.
// - lvl: The logging level of `msg`.
// - fp: Pointer to a null-terminated character array indicating the
//   filepath of the file which invoked writing to `log`.
// - lineno: Positive number of the line on which log writing was invoked.
// - msg: Pointer to a null-terminated character array containing the
//   caller-provided logging message.
//
// Returns: The number of character which would be written to buf if
// bufsz is not heeded, or a negative number on error.
//=======================================================================
static int
prepend_log_info(
		size_t bufsz, char buf[bufsz],
		struct twi_log* log,
		enum twi_log_level lvl,
		const char* fp,
		long lineno,
		const char* msg
) {
	// Argument assertions.
	assertf(buf != NULL, "`buf` points to NULL.");
	assertf(log != NULL, "`log` points to NULL.");
	assertf(lvl != TWI_LOG_LEVEL_NONE,
			"`lvl` is equal to pseudolevel TWI_LOG_LEVEL_NONE, which should "
			"be prevented by the TWI_LOG macro.");
	assertf(lvl <= TWI_LOG_LEVEL_GLOBAL_MAX,
			"`lvl` exceeds the global maximum logging level definition "
			"(%s), which should be prevented by the TWI_LOG macro.",
			log_level_strshort(lvl));
	assertf(fp != NULL, "`fp` points to NULL.");
	assertf(lineno > 0, "`lineno` is less than or equal to zero (%ld)", lineno);
	assertf(msg != NULL, "`msg` points to NULL.");

	if (log->implicit_fp != NULL) {
		fp = trim_prefix(fp, log->implicit_fp);
	}

	// If epoch was successfully initialized, get current time so as to
	// calculate elapsed time. Provide an error string if either the
	// epoch or the current time failed to initialize.
	const char* timestamp_error = NULL;
	struct timespec now;
	if (log->flags & TWI_LOG_FLAG_EPOCH) {
		if (timespec_get(&now, TIME_UTC) != TIME_UTC)
			timestamp_error = "NO_NOW";
	} else
		timestamp_error = "NO_EPOCH";

	int status;
	if (timestamp_error == NULL) {
		// Now and epoch timestamps available.
		// Print elapsed time to the microsecond.
		errno = 0;
		if ((status = snprintf(buf, bufsz,
				"[%lld.%06ld] (%s) %s:%ld: %s\n", // [timestamp] (lvl) fp:lineno: msg
				((long long)difftime(now.tv_sec, log->epoch.tv_sec)),
				(now.tv_nsec - log->epoch.tv_nsec) / 1000,
				log_level_strshort(lvl), fp, lineno, msg
		)) < 0) {
			report_stdio_failure("snprintf() with timestamp", status);
		}
	} else {
		// Either `now` or epoch timestamps are unavailable.
		// Substitute timestamp with error message.
		errno = 0;
		if ((status = snprintf(buf, bufsz,
				"[%s] (%s) %s:%ld: %s\n", timestamp_error,
				log_level_strshort(lvl), fp, lineno, msg
		)) < 0) {
			report_stdio_failure("snprintf() without timestamp", status);
		}
	} // end if/else timestamp aquisition succeeds or fails

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
//
// Parameters:
// - identifier: Pointer to a character array whose contents provide
//   an adequate description of the stdio usage which failed.
// - status: Integral status indicator returned by the offending
//   stdio function.
//=======================================================================
static void
report_stdio_failure(const char* identifier, int status) {
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
// decl resolve_path()
// def resolve_path()
//
// Returns the appropriate pointer to an open stream as indicated by
// the null-terminated character string pointed to by `path`. If this
// function is unable to provide an appropriate stream for the provided
// path, it will instead return NULL.
//
// See the description of twi_log_open_stream() for a detailed
// description of `path` formatting.
//
// If the value of `path` is NULL, behavior is undefined.
//
// Parameters:
// * path:
// Pointer to a null-terminated character string describing
// either a path to which to open an stream, or the name of
// a standard outstream.
// * epoch:
// The time at which this log was opened. Used when resolving requests
// for timestamps.
// * max_index:
// The maximum number of output files that will be kept at once when
// using indexed files. If the creation of a new indexed file would
// cause the oldest file's index to exceed this value, the oldest file
// will instead be deleted. The value of this argument has no effect
// if `path` does not request an index.
//
// Returns:
// Pointer to an outstream appropriate to the value of `path`, or
// NULL if no such stream can be provided.
//=======================================================================
static FILE*
resolve_path(
		const char* path,
		time_t epoch,
		uint8_t max_index
) {
	twi_assertf(path != NULL, "Argument `path` cannot be NULL.");

	// Handle standard outstreams.
	if (strncmp(path, "std", 3) == 0) {
		// If the stream path is "stdout", use existing stdout FILE*.
		// If the stream path is "stderr", use existing stderr FILE*.
		const char* stdtype = path + 3;
		if (strcmp(stdtype, "out") == 0)
			return stdout;
		else if (strcmp(stdtype, "err") == 0)
			return stderr;
	}

	// Perform substitutions on path.
	char resolved_path[max_resolved_pathlen(path) + 1];
} // end resolve_path()

//=======================================================================
// decl rotate_indexed_logs()
// def rotate_indexed_logs()
// // TODO: Description.
//=======================================================================
int_fast8_t
rotate_indexed_logs(
		const char* prefix,
		const char* suffix,
		uint8_t max_files
) {
	assertf(max_files > 0, "Argument `max_files` must be greater than zero.");

	// Check if prefix and suffix together, along with the 3 index characters
	// that will be added between them, are less than the maximum buffer size
	// (accomodating the terminating null byte, of course).
	size_t prefix_len, suffix_len;
	if (prefix == NULL) {
		prefix = "";
		prefix_len = 0;
	} else
		prefix_len = strlen(prefix);
	if (suffix == NULL) {
		suffix = "";
		suffix_len = 0;
	} else
		suffix_len = strlen(suffix);

	if (prefix_len + suffix_len + 3 >= TWI_LOG_PATHNAME_BUFSZ)
		return 1; // Final pathname length exceeds buffer size.
	
	// Check for the existence of each file, from index 0 upward.
	// If a file with an index between 0 and (`max_files` - 1) does not
	// exist, rotation can begin without exceeding the missing file's
	// index.
	uint_fast16_t fileidx;
	for (fileidx = 0; fileidx < max_files; ++fileidx) {
		char pathname[TWI_LOG_PATHNAME_BUFSZ];
		errno = 0;
		int status = snprintf(pathname, TWI_LOG_PATHNAME_BUFSZ,
				"%s%03" PRIuFAST16 "%s", prefix, fileidx, suffix);
		if (status < 0) {
			twi_log_err_libc = "snprintf";
			return -1;
		}

		FILE* tmpfile = fopen(pathname, "r");
		if (tmpfile != NULL)
			fclose(tmpfile);
		else
			break; // File does not exist, and therefore need not be rotated.
	}

	// If all indices up to (`max_files` - 1) exist, the file with the
	// greatest index will be overwritten. Thus, simulate the index of
	// (`max_files` - 1) having no file by subtracting one from `fileidx`.
	if (fileidx >= max_files)
		fileidx -= 1;

	// Now, each file identified to exist before a gap in the index
	// was discovered must be rotated up one index, until the index returns
	// to zero.
	for (; fileidx > 0; --fileidx) {
		// Construct old pathname.
		char old_pathname[TWI_LOG_PATHNAME_BUFSZ];
		errno = 0;
		int status = snprintf(old_pathname, TWI_LOG_PATHNAME_BUFSZ,
				"%s%03" PRIuFAST16 "%s", prefix, fileidx - 1, suffix);
		if (status < 0) {
			twi_log_err_libc = "snprintf";
			return -1;
		}
		// Construct new pathname.
		char new_pathname[TWI_LOG_PATHNAME_BUFSZ];
		errno = 0;
		status = snprintf(new_pathname, TWI_LOG_PATHNAME_BUFSZ,
				"%s%03" PRIuFAST16 "%s", prefix, fileidx, suffix);
		if (status < 0) {
			twi_log_err_libc = "snprintf";
			return -1;
		}
		// Rename
		errno = 0;
		if (rename(old_pathname, new_pathname) != 0) {
			twi_log_err_libc = "rename";
			return -1;
		}
	}

	return 0;
} // end rotate_indexed_logs()

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
//
// Parameters:
// - str: Pointer to a null-terminated character array.
// - prefix: Pointer to a null-terminated character array representing
//   the prefix to trim from the start of `str`, if present.
//
// Returns:
// (str + strlen(prefix)) if the start of the character array
// pointed to by `str` matches the entire character array
// (not including its terminating null byte) pointed to by `prefix`.
// If not, returns `str`.
//=======================================================================
static const char*
trim_prefix(
		const char* str,
		const char* prefix
) {
	assertf(str != NULL, "`str` points to NULL.");
	assertf(prefix != NULL, "`prefix` points to NULL.");

	size_t c;
	for (c = 0; prefix[c]; ++c)
		if (prefix[c] != str[c]) return str;
	return str + c;
} // end trim_prefix()

