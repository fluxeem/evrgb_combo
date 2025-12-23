#include <iostream>
#include "utils/evrgb_logger.h"

using namespace evrgb;

void test_normal_logging() {
    set_log_level(LogLevel::debug);
    LOG_DEBUG("This is a debug message");
    LOG_INFO("This is an info message");
    LOG_WARN("This is a warning message");
    LOG_ERROR("This is an error message");
}

void test_logging_levels() {
    std::cout << "\nSetting log level to INFO" << std::endl;
    set_log_level(LogLevel::info);
    LOG_DEBUG("This debug message should NOT appear");
    LOG_INFO("This is an info message");
    LOG_WARN("This is a warning message");
    LOG_ERROR("This is an error message");

    std::cout << "\nSetting log level to WARN" << std::endl;
    set_log_level(LogLevel::warn);
    LOG_DEBUG("This debug message should NOT appear");
    LOG_INFO("This info message should NOT appear");
    LOG_WARN("This is a warning message");
    LOG_ERROR("This is an error message");
}

int main() {
    test_normal_logging();
    test_logging_levels();
    return 0;
}
