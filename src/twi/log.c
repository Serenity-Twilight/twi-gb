#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <twi/assertf.h>
#include <twi/log.h>

//======================================================================
// decl enum twi_log_flags
// def enum twi_log_flags
//
// Defines flag bits for the twi_log.flags variable.
// TODO: Improve and complete documenation.
//======================================================================
enum twi_log_flags {
	// If set, indicates that the epoch of the current log was
	// successfully set during initialization. If cleared, indicates
	// that initialization failed to set the epoch, and that it's
	// value is undefined.
	TWI_LOG_FLAG_EPOCH = 0x01,
	TWI_LOG_FLAG_IMPLICIT_FP_ALLOC = 0x02
};


//======================================================================
// decl struct twi_log_timestamp
// def struct twi_log_timestamp
//
// The twi_log timestamp, represented by a number of days, hours
// minutes, seconds, and microseconds. Each time unit less than a day
// is wrapped so that its total value is less than a value which would
// be equivalent to at least one of a greater time unit.
//
// micros is bound between the values of 0-99999 inclusive.
// seconds is bound between the values of 0-59 inclusive.
// mintues is bound between the values of 0-59 inclusive.
// hours is bound between the values of 0-23 inclusive.
//======================================================================
struct twi_log_timestamp {
	uint32_t days;
	uint32_t micros;
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
};

//======================================================================
//======================================================================
// Forward declarations of internal functions.
//======================================================================
//======================================================================
static const char*
get_elasped_timestamp(struct twi_log_timestamp*, struct twi_log*);
static const char*
log_level_strshort(enum twi_log_level);
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
static const char*
trim_prefix(const char*, const char*);

//======================================================================
//======================================================================
// External functions
//======================================================================
//======================================================================

//=======================================================================
// def twi_log_init()
//=======================================================================
void
twi_log_init(
		struct twi_log* log,
		enum twi_log_level max_lvl
) {
	assertf(log != NULL, "Argument `log` points to NULL.");
	assertf(max_lvl >= TWI_LOG_LEVEL_NONE
			&& max_lvl <= TWI_LOG_LEVEL_TRACE,
			"Argument `max_lvl` is not a recognized logging level (%d)",
			max_lvl);

	log->stream = NULL;
	log->implicit_fp = NULL;
	log->max_lvl = max_lvl;

	// Set/clear TWI_LOG_FLAG_EPOCH on success/failure.
	if (timespec_get(&(log->epoch), TIME_UTC) == TIME_UTC)
		log->flags = TWI_LOG_FLAG_EPOCH;
	else
		log->flags = 0;
} // end twi_log_init()

//=======================================================================
// def twi_log_destroy()
//=======================================================================
void
twi_log_destroy(struct twi_log* log) {
	assertf(log != NULL, "`log` points to NULL.");
	twi_log_stream_close(log);
} // end twi_log_destroy()

//=======================================================================
// def twi_log_write()
//=======================================================================
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
// def twi_log_stream_open()
//======================================================================
int
twi_log_stream_open(
		struct twi_log* log,
		const char* filename
) {
	assertf(log != NULL, "Argument `log` points to NULL.");
	assertf(filename != NULL, "Argument `filename` points to NULL.");

	if (log->stream != NULL)
		twi_log_stream_close(log);
	log->stream = fopen(filename, "w");
	if (log->stream == NULL)
		return 1; // Failed to open stream.

	// TODO: Begin log file with system info.
	return 0;
} // end twi_log_stream_open()

//======================================================================
// def twi_log_stream_close()
//======================================================================
void
twi_log_stream_close(struct twi_log* log) {
	assertf(log != NULL, "Argument `log` points to NULL.");

	// TODO: Maybe "log closed normally" message?
	if (log->stream != NULL) {
		fclose(log->stream);
		log->stream = NULL;
	}
} // end twi_log_stream_close()

//======================================================================
//======================================================================
// Internal functions
//======================================================================
//======================================================================

//======================================================================
// decl get_elapsed_timestamp()
// def get_elapsed_timestamp()
//
// Calculates time elapsed between now and the epoch stored within
// `log`, and stores the result in `elapsed`.
//
// This function will be unable to calculate an elapsed time if either
// `log` failed to initialize its epoch or this function cannot
// retrieve the current time for whatever reason. In the case of failure,
// the argument `elapsed` is not modified.
//
// Parameters:
// - elapsed: A pointer to a twi_log_timestamp used to store the time
//   elapsed between now and the provided `log`'s epoch.
// - log: A pointer to an initialized twi_log whose epoch is used as
//   the starting time by which to calculate how much time has elapsed.
// 
// Returns: NULL on success, or a pointer to the start of a
// null-terminated character array whose contents describes the cause
// of the error is no more than 15 characters, intended to be used
// as a substitution for the timestamp.
//======================================================================
static const char*
get_elapsed_timestamp(
		struct twi_log_timestamp* elapsed,
		const struct twi_log* log
) {
	assertf(elapsed, "Argument `elapsed` points to NULL.");
	assertf(log, "Argument `log` points to NULL.");

	if (log->flags & TWI_LOG_FLAG_EPOCH) {
		struct timespec now;
		if (timespec_get(&now, TIME_UTC) == TIME_UTC) {
			// Contains the unwrapped timestamp value in seconds.
			uint64_t tmp_timestamp =
				(uint64_t)difftime(now.tv_sec, log->epoch.tv_sec);
			
			// Ensure nanosecond count is positive, and convert to microseconds.
			int32_t tmp_nano = now.tv_nsec - log->epoch.tv_nsec;
			if (tmp_nano < 0) {
				tmp_timestamp -= 1;
				elapsed->micros = (tmp_nano + 1000000000) / 1000;
			} else {
				elapsed->micros = tmp_nano / 1000;
			}

			// Derive wrapped timestamp values from the unwrapped tmp_timestamp.
			elapsed->seconds = tmp_timestamp % 60;
			tmp_timestamp /= 60; // Convert to minutes.
			elapsed->minutes = tmp_timestamp % 60;
			tmp_timestamp /= 60; // Convert to hours.
			elapsed->hours = tmp_timestamp % 24;
			elapsed->days = tmp_timestamp / 24;
		} else {
			// timespec_get failed to get current time.
			// Determining elapsed time is impossible.
			return "NOW UNKNOWN";
		}
	} else {
		// Provided log's epoch failed to initialize.
		// Therefore, determining elapsed time is impossible.
		return "EPOCH UNKNOWN";
	}

	return NULL;
} // end get_elapsed_timestamp()

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

