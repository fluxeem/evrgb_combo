#ifndef EVRGB_LOGGER_H
#define EVRGB_LOGGER_H

#ifdef _WIN32
    #ifdef EVRGB_EXPORTS
        #define EVRGB_API __declspec(dllexport)
    #else
        #define EVRGB_API __declspec(dllimport)
    #endif
#else
    #define EVRGB_API   
#endif

#include <loguru.hpp>

namespace evrgb {

enum class EVRGB_API LogLevel {
    trace = 0,
    debug = 1,
    info = 2,
    warn = 3,
    err = 4,
    critical = 5,
    off = 6
};

// Set log level using enum.
EVRGB_API void set_log_level(LogLevel level);

// Set log level using name (case-insensitive):
// "trace", "debug", "info", "warn", "error", "critical", "off".
// Returns true if applied, false if name is invalid.
EVRGB_API bool set_log_level_by_name(const char* level_name);

} // namespace evrgb

// Map to loguru macros
// Note: loguru uses printf-style formatting (%s, %d), not fmt-style ({})
#define EVRGB_LOG_TRACE(...)    LOG_F(2, __VA_ARGS__)
#define EVRGB_LOG_DEBUG(...)    LOG_F(1, __VA_ARGS__)
#define EVRGB_LOG_INFO(...)     LOG_F(INFO, __VA_ARGS__)
#define EVRGB_LOG_WARN(...)     LOG_F(WARNING, __VA_ARGS__)
#define EVRGB_LOG_ERROR(...)    LOG_F(ERROR, __VA_ARGS__)
#define EVRGB_LOG_CRITICAL(...) LOG_F(FATAL, __VA_ARGS__)

// Short aliases
#define LOG_TRACE(...)    EVRGB_LOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG(...)    EVRGB_LOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...)     EVRGB_LOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)     EVRGB_LOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...)    EVRGB_LOG_ERROR(__VA_ARGS__)
#define LOG_CRITICAL(...) EVRGB_LOG_CRITICAL(__VA_ARGS__)

#endif // EVRGB_LOGGER_H
