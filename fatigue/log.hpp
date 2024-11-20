#pragma once

#include <chrono>
#include <filesystem>
#include <format>
#include <iostream>
#include <map>
#include <source_location>
#include "utils.hpp"

using namespace fatigue::color;

namespace fatigue::log {
    /**
     * Log levels for messages
     */
    enum class LogLevel: int {
        Quiet = -1,
        Error,
        Warning,
        Success,
        Fail,
        Info,
        Debug,
    };

    void setLogLevel(LogLevel lvl);
    LogLevel getLogLevel();

    /**
     * Log format options
     */
    enum class LogFormat: int {
        Verbose, // currently unused
        Default,
        Compact,
        Tiny,
        NoLabel,
    };

    void setLogFormat(LogFormat fmt);
    LogFormat getLogFormat();

    const std::map<LogLevel, const std::string> logLevelNames = {
        {LogLevel::Error, "💥 ERROR"},
        {LogLevel::Warning, "🚩 WARNING"},
        {LogLevel::Fail, "❌ FAIL"},
        {LogLevel::Success, "✔️ SUCCESS"},
        {LogLevel::Info, "💬 INFO"},
        {LogLevel::Debug, "🔨 DEBUG"},
    };

    const std::map<LogLevel, Color> logLevelColors = {
        {LogLevel::Error, Color::BrightRed},
        {LogLevel::Warning, Color::BrightYellow},
        {LogLevel::Fail, Color::Red},
        {LogLevel::Success, Color::Green},
        {LogLevel::Info, Color::BrightBlue},
        {LogLevel::Debug, Color::Magenta},
    };

    std::string logColorize(LogLevel const lvl, std::string const &str);

    // Type to string formatters

    std::string to_string(LogLevel const lvl);
    std::string to_tag(LogLevel const lvl);

    auto as_local(std::chrono::system_clock::time_point const tp);
    std::string to_string(std::chrono::system_clock::time_point const tp);

    std::string to_string(std::source_location const source);

    // Logging functions

    void log(LogLevel const lvl, std::string_view const message,
                    std::source_location const source = std::source_location::current());
}

// Convenience macros for logging
#ifndef logError
#define logError(msg) fatigue::log::log(fatigue::log::LogLevel::Error, msg);
#endif
#ifndef logWarning
#define logWarning(msg) fatigue::log::log(fatigue::log::LogLevel::Warning, msg);
#endif
#ifndef logSuccess
#define logSuccess(msg) fatigue::log::log(fatigue::log::LogLevel::Success, msg);
#endif
#ifndef logFail
#define logFail(msg) fatigue::log::log(fatigue::log::LogLevel::Fail, msg);
#endif
#ifndef logInfo
#define logInfo(msg) fatigue::log::log(fatigue::log::LogLevel::Info, msg);
#endif
#ifndef logDebug
#define logDebug(msg) fatigue::log::log(fatigue::log::LogLevel::Debug, msg);
#endif
