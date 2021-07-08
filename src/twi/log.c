#include <stdarg.h>
#include <stdbool.h>
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
//======================================================================
enum twi_log_flags {
	// If set, indicates that the epoch of the current log was
	// successfully set during initialization. If cleared, indicates
	// that initialization failed to set the epoch, and that it's
	// value is undefined.
	TWI_LOG_FLAG_EPOCH = 0x01
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
	uint64_t days;
	uint32_t micros;
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
};

//======================================================================
// decl twi_log_get_elapsed_time()
// def twi_log_get_elapsed_time()
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
// TODO: More failure info. Add some information to the main
// description, as well.
// Returns: 0 on success, or a non-zero value on failure.
//======================================================================
static int
twi_log_get_elapsed_time(
		struct twi_log_timestamp* elapsed,
		const struct twi_log* log
) {
	assertf(elapsed, "Argument `elapsed` points to NULL.");
	assertf(log, "Argument `log` points to NULL.");

	if (log->flags & TWI_LOG_FLAGS_EPOCH) {
		struct timespec now;
		if (timespec_get(&now, TIME_UTC) == TIME_UTC) {
			// Contains the unwrapped timestamp value in seconds.
			uint64_t tmp_timestamp =
				(uint64_t)difftime(now->tv_sec, log->epoch.tv_sec);
			
			// Ensure nanosecond count is positive, and convert to microseconds.
			int32_t tmp_nano = nov.tv_nsec - log->epoch.tv_nsec;
			if (tmp_nano < 0) {
				tmp_timestamp -= 1;
				elapsed.micros = (tmp_nano + 1000000000) / 1000;
			} else {
				elapsed.micros = tmp_nano / 1000;
			}

			// Derive wrapped timestamp values from the unwrapped tmp_timestamp.
			elapsed.seconds = tmp_timestamp % 60;
			tmp_timestamp /= 60; // Convert to minutes.
			elapsed.minutes = tmp_timestamp % 60;
			tmp_timestamp /= 60; // Convert to hours.
			elapsed.hours = tmp_timestamp % 24;
			elapsed.days = tmp_timestamp / 24;
		} else {
			// timespec_get failed to get current time.
			// Determining elapsed time is impossible.
			return 1;
		}
	} else {
		// Provided log's epoch failed to initialize.
		// Therefore, determining elapsed time is impossible.
		return 1;
	}

	return 0;
} // end twi_log_get_elapsed_time()

//=======================================================================
// decl twi_log_level_string()
// def twi_log_level_string()
//
// Provides a human-readable string version of the provided
// twi_log_level in all capitals.
//
// If an invalid value is provided, then the string "UNKNOWN" is
// is returned.
//
// Parameter:
// * level - A twi_log_level.
// Returns:
// A pointer to a statically allocated, null-terminated character
// array containing a human-readable string version of `level`.
//=======================================================================
static const char*
twi_log_level_string(enum twi_log_level level) {
	switch (level) {
		case TWI_LOG_LEVEL_NONE: return "NONE";
		case TWI_LOG_LEVEL_FATAL: return "FATAL";
		case TWI_LOG_LEVEL_ERROR: return "ERROR";
		case TWI_LOG_LEVEL_WARN: return "WARN";
		case TWI_LOG_LEVEL_INFO: return "INFO";
		case TWI_LOG_LEVEL_DEBUG: return "DEBUG";
		case TWI_LOG_LEVEL_TRACE: return "TRACE";
		default: return "UNKNOWN";
	}
}

//=======================================================================
// decl trim_filename_prefix()
// def trim_filename_prefix()
//
// If the entire character array (excluding terminating null byte)
// pointed to by `prefix` matches the start of the character array
// pointed to by `filename`, this function returns a character pointer
// to the first character of `filename` following the character matching
// those of `prefix`, effectively trimming the matching prefix.
//
// If the contents of `prefix` and the start of `filename` do not match
// exactly, then `filename` is returned.
//
// Under no circumstances are any strings modified by this function.
// If `filename` points to the start of dynamically allocated memory,
// ensure that the original pointer is used for deallocation.
//
// Parameters:
// - prefix: Pointer to a null-terminated character array representing
//   the prefix to trim from the start of `filename`, if they match.
// - filename: Pointer to a null-terminated character array representing
//   a filepath to trim `prefix` from the start of, if they match.
//
// Returns:
// (filename + strlen(prefix)) if the start of the character array
// pointed to by `filename` matches the entire character array
// (not including its terminating null byte) pointed to by `prefix`.
// If not, returns (filename).
//=======================================================================
const char* trim_filename_prefix(
		const char* prefix,
		const char* filename
) {
	unsigned int c;
	for (c = 0; prefix[c]; ++c)
		if (prefix[c] != filename[c]) return filename;
	return filename + c;
} // end trim_filename_prefix()

//=======================================================================
// def twi_log_write()
//
// TODO: Meaningful internal description.
//=======================================================================
void
twi_log_write(
		struct twi_log* log,
		enum twi_log_level level,
		const char* filename,
		long line,
		const char* msg,
		...
) {
	assertf(log, "Argument `log` points to NULL.");
	assertf(filename, "Argument `filename` points to NULL.");
	assertf(line >= 0, "Argument `line` is negative (%ld).", line);
	assertf(msg, "Argument `msg` points to NULL.");

	// If no target file exists, do nothing.
	if (!log->target)
		return;

	if (log->trim_filename_prefix)
		filename = trim_filename_prefix(log->trim_filename_prefix, filename);

	// If one of the calls of an stdio function fails, the following
	// character pointer will be assigned the address of a string
	// uniquely identifying which call caused the failure.
	const char* stdio_failure;
	char buf[BUFSZ];
	int status;
	{
		struct twi_log_timestamp elapsed;
		if (!(status = twi_log_get_elapsed_time(&elapsed, log))) {
			// If at least one day has been elapsed, days will be displayed on the timestamp.
			// As changes to the number of days rarely disrupts the otherwise constant width
			// of the timestamp, no minimum allocated width is used.
			int days_max_width;
			const char* days_separator;
			if (elapsed.days > 0) {
				// Show day counter and separator.
				days_max_width = INT_MAX;
				days_separator = ":";
			} else {
				// Hide day counter and separator.
				days_max_width = 0;
				days_separator = "";
			}

			// snprintf() is not required by the C Standard to set errno on error.
			// But if this implementation does, clear errno beforehand so that
			// it can be determined if snprintf() set it.
			errno = 0;
			if ((status = snprintf(buf, BUFSZ,
					"[%.*" PRIu64 "%s%02" PRIu8 ":%02" PRIu8 ":%02" PRIu8 ".%06" PRIu32 // Timestamp
					"] (%s):%s:%ld: %s", // (level):filename:line: msg
					days_max_width, elapsed.days, days_separator,
					elapsed.hours, elapsedminutes, elapsed.seconds, elapsed.micros,
					twi_log_level_string(level), filename, line, msg
			)) < 0) {
				stdio_failure = "snprintf() with timestamp";
				goto stdio_report_error;
			}
		} else {
			// Elapsed time could not be determined.
			// Substitute timestamp with a string indicating the reason for failure.
			// The error string should be the same width as a standard timestamp
			// without days in order to maintain horizontal alignment with
			// functioning timestamps, if any exist.
			const char* timestamp_error_msg;
			switch(status) {
				case 1:
					timestamp_error_msg = "EPOCH UNKNOWN";
					break;
				case 2:
					timestamp_error_msg = "NOW UNKNOWN";
					break;
			}

			// snprintf() is not required by the C Standard to set errno on error.
			// But if this implementation does, clear errno beforehand so that
			// it can be determined if snprintf() set it.
			errno = 0;
			if ((status = snprintf(buf, BUFSZ,
					"[%-15s] (%s):%s:%ld: %s", timestamp_error_msg,
					twi_log_level_string(level), filename, line, msg
			)) < 0) {
				stdio_failure = "snprintf() without timestamp";
				goto stdio_report_error;
			}
		}
	}

	// Substitute caller-provided arguments into the caller's message string
	// and write to target.
	va_list caller_args;
	va_start(caller_args, msg);
	// vfprintf() is not required by the C Standard to set errno on error.
	// But if this implementation does, clear errno beforehand so that
	// it can be determined if vfprintf() set it.
	errno = 0;
	status = vfprintf(log->target, buf, caller_args);
	va_end(caller_args);
	if (status < 0) {
		stdio_failure = "vfprintf()";
		goto stdio_report_error;
	}
	return;

stdio_report_error:
	// Attempt to report error, even though it will also most likely fail.
	// The result of this output is irrelevant, as there is nothing more
	// that can be done if the error cannot be reported through standard
	// streams.
	if (errno)
		fprintf(stderr, "twi_log_write(): %s returned %d\n"
				"\terrno = %d (\"%s\")\n",
				stdio_failure, status, errno, strerror(errno));
	else
		fprintf(stderr, "twi_log_write(): %s() returned %d, errno not set\n",
				stdio_failure, status);
} // end twi_log_write()
