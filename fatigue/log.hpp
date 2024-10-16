#pragma once

#ifndef FATIGUE_LOG_LEVEL
#define FATIGUE_LOG_LEVEL fatigue::log::LogLevel::Warning
#endif

#ifndef FATIGUE_LOG_COMPACT
#define FATIGUE_LOG_COMPACT false
#endif

#include <chrono>
#include <filesystem>
#include <iostream>
#include <map>
#include <source_location>
#include <string>
#include "utils.hpp"

using namespace fatigue::color;

namespace fatigue::log {
    /**
     * Log levels for messages
     * Set FATIGUE_LOG_LEVEL to the desired level before including this file
     *     If set to Debug, messages will include source location
     * Set FATIGUE_LOG_COMPACT to true to disable timestamps and newlines
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

    const std::map<LogLevel, const std::string> logLevelNames = {
        {LogLevel::Error, "💥 ERROR"},
        {LogLevel::Warning, "🚩 WARNING"},
        {LogLevel::Fail, "➖ FAIL"},
        {LogLevel::Success, "➕ SUCCESS"},
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

    inline std::string logColorize(LogLevel const lvl, std::string const &str)
    {
        return colorize(logLevelColors.contains(lvl) ? logLevelColors.at(lvl) : Color::Reset, str);
    }

    inline std::string to_string(LogLevel const lvl)
    {
        return logLevelNames.contains(lvl) ? logLevelNames.at(lvl) : "?";
    }

    inline std::string to_tag(LogLevel const lvl)
    {
        return logColorize(lvl, std::format("[ {:<10} ]", to_string(lvl)));
    }

    inline auto as_local(std::chrono::system_clock::time_point const tp)
    {
        return std::chrono::zoned_time{std::chrono::current_zone(), tp};
    }

    inline std::string to_string(std::chrono::system_clock::time_point const tp)
    {
        return std::format("{:%F %T %Z}", tp);
    }

    inline std::string to_string(std::source_location const source)
    {
        return std::format(
            "{} in {}:{}",
            source.function_name(),
            std::filesystem::path(source.file_name()).filename().string(),
            source.line()
        );
    }

    inline void log(LogLevel const lvl, std::string_view const message,
                    std::source_location const source = std::source_location::current())
    {
        // Only log messages at or above the specified level
        if (lvl < LogLevel::Error || lvl > FATIGUE_LOG_LEVEL) return;

        // Start header
        std::cout << to_tag(lvl) << ' ';

        // Compact mode: no timestamp, single line
        if (!FATIGUE_LOG_COMPACT) {
            std::cout << Color::BrightBlack << to_string(as_local(std::chrono::system_clock::now())) << Color::Reset;
            std::cout << std::endl;
        }
        // End header

        // All levels: message
        std::cout << message << std::endl;

        // Debug: source location for debug level (all levels if FATIGUE_LOG_LEVEL == Debug)
        if (FATIGUE_LOG_LEVEL >= LogLevel::Debug) {
            std::cout << Color::BrightBlack << "🔍️ At " << to_string(source) << Color::Reset << std::endl;
        }

        if (!FATIGUE_LOG_COMPACT) {
            std::cout << std::endl;
        }
    }
}

#define logError(msg) fatigue::log::log(fatigue::log::LogLevel::Error, msg);
#define logWarning(msg) fatigue::log::log(fatigue::log::LogLevel::Warning, msg);
#define logSuccess(msg) fatigue::log::log(fatigue::log::LogLevel::Success, msg);
#define logFail(msg) fatigue::log::log(fatigue::log::LogLevel::Fail, msg);
#define logInfo(msg) fatigue::log::log(fatigue::log::LogLevel::Info, msg);
#define logDebug(msg) fatigue::log::log(fatigue::log::LogLevel::Debug, msg);
