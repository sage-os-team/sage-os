#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <am.h>

/**
 * @brief console colors
 */
#define FONT_NORMAL "\033[0m"
#define FONT_BOLD   "\033[1m"

#define FG_BLACK  "\033[1;30m"
#define FG_RED    "\033[1;31m"
#define FG_GREEN  "\033[1;32m"
#define FG_YELLOW "\033[1;33m"
#define FG_BLUE   "\033[1;34m"
#define FG_PURPLE "\033[1;35m"
#define FG_CYAN   "\033[1;36m"
#define FG_WHITE  "\033[1;37m"

#define BG_BLACK  "\033[1;40m"
#define BG_RED    "\033[1;41m"
#define BG_GREEN  "\033[1;42m"
#define BG_YELLOW "\033[1;43m"
#define BG_BLUE   "\033[1;44m"
#define BG_PURPLE "\033[1;45m"
#define BG_CYAN   "\033[1;46m"
#define BG_WHITE  "\033[1;47m"

#define COLOR_NONE FONT_NORMAL

/**
 * @brief Debug log control bit
 */
#define LOG_SUCCESS 0x01
#define LOG_INFO    0x02
#define LOG_WARN    0x04
#define LOG_ERROR   0x08

/**
 * @brief LOG_MASK control whether to output log message.
 *
 * Function log() will print the message only if
 * (type & LOG_MASK == true).
 *
 *  0: disable
 * 16: all
 * 12: only WARN and ERROR
 *  3: only INFO and SUCCESS
 *
 */
int _log_mask;

int logger_lock;

#ifndef LOG_MASK
#define LOG_MASK \
  (LOG_INFO | LOG_WARN | LOG_ERROR | \
   LOG_SUCCESS)  // defalt print INFO and above
#endif

extern const char* logger_type_str[10];

#ifdef NODEBUG

#define log(type, format, ...)        ;
#define log_detail(type, format, ...) ;

#else

#define _LOCK \
  ({ \
    while (atomic_xchg(&logger_lock, 1)) \
      ; \
  })

#define _UNLOCK ({ asm volatile("movl $0, %0" : "+m"(logger_lock) :); })

#define log(type, format, ...) \
  if (type & _log_mask) { \
    _LOCK; \
    printf("#%d %s: " format "\n", cpu_current(), logger_type_str[type], \
           ##__VA_ARGS__); \
    _UNLOCK; \
  }

#define log_detail(type, format, ...) \
  if (type & _log_mask) { \
    _LOCK; \
    printf("#%d %s (%s:%d, %s):\n" format "\n", cpu_current(), \
           logger_type_str[type], __FILE__, __LINE__, __func__, \
           ##__VA_ARGS__); \
    _UNLOCK; \
  }

#endif

#define success(format, ...) log(LOG_SUCCESS, format, ##__VA_ARGS__)
#define info(format, ...)    log(LOG_INFO, format, ##__VA_ARGS__)
#define warn(format, ...)    log(LOG_WARN, format, ##__VA_ARGS__)
#define error(format, ...)   log(LOG_ERROR, format, ##__VA_ARGS__)
#define success_detail(format, ...) \
  log_detail(LOG_SUCCESS, format, ##__VA_ARGS__)
#define info_detail(format, ...)  log_detail(LOG_INFO, format, ##__VA_ARGS__)
#define warn_detail(format, ...)  log_detail(LOG_WARN, format, ##__VA_ARGS__)
#define error_detail(format, ...) log_detail(LOG_ERROR, format, ##__VA_ARGS__)

#define assert_msg_block(cond, format, ...) \
  if (!(cond)) { \
    printf("#%d " FG_RED "ASSERT_FAILED" COLOR_NONE "(%s:%d, %s):\n" format \
           "\n", \
           cpu_current(), __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
    while (1) \
      ; \
  }

#define assert_msg(cond, format, ...) \
  if (!(cond)) \
    printf("#%d " FG_RED "ASSERT_FAILED" COLOR_NONE "(%s:%d, %s):\n" format \
           "\n", \
           cpu_current(), __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
  assert((cond));

/**
 * @brief Control whether enable function trace
 */
#ifdef NODEBUG
#define TRACE_ENTRY ((void)0)
#define TRACE_EXIT  ((void)0)
#else
#define TRACE_ENTRY info("[trace] %s:entry\n", __func__)
#define TRACE_EXIT  info("[trace] %s:exit\n", __func__)
#endif

#endif
