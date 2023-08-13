#pragma once
#include "pch.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

class Logger
{
public:
    Logger();
    Logger(const char* loggerName);

    void doNothing(){};
    std::shared_ptr<spdlog::logger> getLogger() { return logger; }
private:
    std::shared_ptr<spdlog::logger> logger;
};

#define TRACE(...) getLogger()->trace(__VA_ARGS__)
#define INFO(...) getLogger()->info(__VA_ARGS__)
#define WARN(...) getLogger()->warn(__VA_ARGS__)
#define ERROR(...) getLogger()->error(__VA_ARGS__)