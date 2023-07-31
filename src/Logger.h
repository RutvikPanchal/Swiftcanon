#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <memory>

class Logger
{
public:
    Logger();
    Logger(const char* loggerName);

    template <typename... Args> void trace(const Args&... args) { logger->trace(args...); }
    template <typename... Args> void info(const Args&... args)  { logger->info(args...);  }
    template <typename... Args> void warn(const Args&... args)  { logger->warn(args...);  }
    template <typename... Args> void error(const Args&... args) { logger->error(args...); }
private:
    std::shared_ptr<spdlog::logger> logger;
};