#include "utils/evrgb_logger.h"

#include <algorithm>
#include <string>
#include <cctype>

namespace 
{
    struct LoguruGlobalParaSet
    {
        LoguruGlobalParaSet()
        {
            loguru::g_preamble_date = false;
            loguru::g_preamble_uptime = false;
            loguru::g_preamble_thread = false;
        }
    } loguru_global_para_set;
    
} // namespace 


namespace evrgb { 

void set_log_level(LogLevel level) {
    loguru::Verbosity v = loguru::Verbosity_INFO;
    switch (level) {
        case LogLevel::trace: v = 2; break;
        case LogLevel::debug: v = 1; break;
        case LogLevel::info: v = loguru::Verbosity_INFO; break;
        case LogLevel::warn: v = loguru::Verbosity_WARNING; break;
        case LogLevel::err: v = loguru::Verbosity_ERROR; break;
        case LogLevel::critical: v = loguru::Verbosity_FATAL; break;
        case LogLevel::off: v = loguru::Verbosity_OFF; break;
        default: v = loguru::Verbosity_INFO; break;
    }
    loguru::g_stderr_verbosity = v;
}

bool set_log_level_by_name(const char* level_name) {
    if (!level_name) return false;
    std::string name(level_name);
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c){ return std::tolower(c); });
    if (name == "trace") { set_log_level(LogLevel::trace); return true; }
    if (name == "debug") { set_log_level(LogLevel::debug); return true; }
    if (name == "info") { set_log_level(LogLevel::info); return true; }
    if (name == "warn" || name == "warning") { set_log_level(LogLevel::warn); return true; }
    if (name == "error" || name == "err") { set_log_level(LogLevel::err); return true; }
    if (name == "critical" || name == "fatal") { set_log_level(LogLevel::critical); return true; }
    if (name == "off" || name == "none") { set_log_level(LogLevel::off); return true; }
    return false;
}

} // namespace evrgb
