#include <logger.h>

const char* type_str[10] = {
    [LOG_SUCCESS] = FG_GREEN "SUCCESS" COLOR_NONE,
    [LOG_INFO]    = FG_CYAN "INFO" COLOR_NONE,
    [LOG_WARN]    = FG_PURPLE "WARN" COLOR_NONE,
    [LOG_ERROR]   = FG_RED "ERROR" COLOR_NONE,
};