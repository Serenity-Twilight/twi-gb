#include <stdlib.h>

#include <twi/gb/log.h>

int main(int argc, char* argv[]) {
	if (twi_gb_stdlog_init(TWI_LOG_LEVEL_GLOBAL_MAX, TWI_GB_PROGNAME, "log", TWI_GB_MAX_LOGS))
		return EXIT_FAILURE;

	setvbuf(twi_gb_stdlog.stream, NULL, _IONBF, 0);
	
	int subst_int = 500;
	const char* subst_str = "This is dynamically written! Wow!";
	LOGD("TWI_GB_PROGNAME = " TWI_GB_PROGNAME);
	LOGE("TWI_LOG_LEVEL_TRACE = %d, TWI_LOG_LEVEL_GLOBAL_MAX = %d",
			TWI_LOG_LEVEL_TRACE, TWI_LOG_LEVEL_GLOBAL_MAX);
	LOGF("This is a logging message with level FATAL. No substitutions.");
	LOGE("This is a logging message with level ERROR. No substitutions.");
	LOGW("This is a logging message with level WARN. No substitutions.");
	LOGI("This is a logging message with level INFO. No substitutions.");
	LOGD("This is a logging message with level DEBUG. No substitutions.");
	LOGT("This is a logging message with level TRACE. No substitutions.");
	LOGT("Substitutions:\nint = %d\nstr = %s", subst_int, subst_str);

	twi_gb_stdlog_destroy();
	return EXIT_SUCCESS;
}
