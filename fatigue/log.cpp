#include "log.hpp"

namespace fatigue::log {
    // Global options

    LogLevel s_globalLogLevel{LogLevel::Warning};

    void setLogLevel(LogLevel lvl) { s_globalLogLevel = lvl; }
    LogLevel getLogLevel() { return s_globalLogLevel; }

    LogFormat s_globalLogFormat{LogFormat::Default};

    void setLogFormat(LogFormat fmt) { s_globalLogFormat = fmt; }
    LogFormat getLogFormat() { return s_globalLogFormat; }

    // Type to string formatters

    std::string logColorize(LogLevel const lvl, std::string const &str)
    {
        return colorize(logLevelColors.contains(lvl) ? logLevelColors.at(lvl) : Color::Reset, str);
    }

    std::string to_string(LogLevel const lvl)
    {
        return logLevelNames.contains(lvl) ? logLevelNames.at(lvl) : "?";
    }

    std::string to_tag(LogLevel const lvl)
    {
        std::format_string<std::string> fmt{"{}"};
        switch (getLogFormat()) {
            case LogFormat::Tiny:
                // Only use the first letter (emoji) of the log level
                fmt = "{:.2s} ";
                break;
            case LogFormat::Compact:
                // Pad the tag to equal width so messages stay aligned
                fmt = "[ {:^10} ]";
                break;
            default:
                fmt = "[ {} ]";
                break;
        }

        return logColorize(lvl, std::format(fmt, to_string(lvl)));
    }

    auto as_local(std::chrono::system_clock::time_point const tp)
    {
        return std::chrono::zoned_time{std::chrono::current_zone(), tp};
    }

    std::string to_string(std::chrono::system_clock::time_point const tp)
    {
        return std::format("{:%F %T %Z}", tp);
    }

    std::string to_string(std::source_location const source)
    {
        return std::format(
            "{} in {}:{}",
            source.function_name(),
            std::filesystem::path(source.file_name()).filename().string(),
            source.line()
        );
    }

    /**
     * Log a message with the specified level, using global options for log level and format
     */
    void log(LogLevel const lvl, std::string_view const message, std::source_location const source)
    {
        // Only log messages at or above the specified level
        if (lvl < LogLevel::Error || lvl > getLogLevel()) return;

        // For warnings and errors, output to cerr, else to cout
        std::ostream &out = (lvl == LogLevel::Error || lvl == LogLevel::Warning) ? std::cerr : std::cout;

        // Start header
        out << to_tag(lvl) << ' ';

        // In default or verbose, output timestamp and a linebreak
        if (getLogFormat() <= LogFormat::Default) {
            out << Color::BrightBlack << to_string(as_local(std::chrono::system_clock::now())) << Color::Reset;
            out << std::endl;
        }
        // End header

        // All levels: message
        // if error or warning, output just the message to cerr, else to cout
        out << message << std::endl;

        // Show source location in debug level (all levels if verbose)
        if (lvl == LogLevel::Debug || getLogFormat() == LogFormat::Verbose) {
            out << Color::BrightBlack << "🔍️ At " << to_string(source) << Color::Reset << std::endl;
        }

        // In default or verbose, output an additional linebreak to separate messages
        if (getLogFormat() <= LogFormat::Default) {
            out << std::endl;
        }
    }
} // namespace fatigue
