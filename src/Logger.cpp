#include "Logger.h"

Logger::Logger()
{
    logger = spdlog::stdout_color_mt("LOG");
    logger->set_pattern("%^%T [%n] %v%$");
    logger->set_level(spdlog::level::trace);
}

Logger::Logger(const char* loggerName)
{
    logger = spdlog::stdout_color_mt(loggerName);
    logger->set_level(spdlog::level::trace);
    logger->set_pattern("%^%T [%n] %v%$");
}