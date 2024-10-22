#pragma once

#ifndef NO_COLOR
#define NO_COLOR false
#endif

#include <iostream>
#include <string>

namespace fatigue {
    namespace color {
        /** @brief ANSI color codes by name (FG only) */
        enum class Color: int {
            Reset = 0, Bold, Dim, Underline,
            Black = 30, Red, Green, Yellow, Blue, Magenta, Cyan, White,
            BrightBlack = 90, BrightRed, BrightGreen, BrightYellow, BrightBlue, BrightMagenta, BrightCyan, BrightWhite
        };

        inline std::ostream &operator<<(std::ostream &os, Color code)
        {
            if (NO_COLOR) return os;
            return os << "\033[" << static_cast<int>(code) << "m";
        }

        /** @brief Colorize a string with ANSI color codes */
        inline std::string colorize(Color code, std::string const &str)
        {
            if (NO_COLOR) return str;
            return "\033[" + std::to_string(static_cast<int>(code)) + "m" + str + "\033[0m";
        }
    } // namespace color

    namespace string {
        /** Return an uppercase copy of a string */
        std::string toUpper(const std::string &str);
        /** Return a lowercase copy of a string */
        std::string toLower(const std::string &str);
        /** Copy a string and trim leading and trailing spaces and unprintable characters */
        std::string trim(std::string &str);
    }

    namespace hex {
        // Alias some very useful functions from KittyUtils
        // constexpr auto validateHex = KittyUtils::String::ValidateHex;

        /** Convert a chunk of data to its hexadecimal representation */
        std::string toHex(const void* data, const std::size_t length);
        /** Convert a chunk of data to its hexadecimal representation, autosize */
        template <typename T>
        std::string toHex(const T &data) { return toHex(data, sizeof(T)); }
        // /** Convert a hexadecimal string to a chunk of data */
        // constexpr auto dataFromHex = KittyUtils::dataFromHex;

        // /** Print HEX string from data; formatted uppercase with spaces between each byte pair */
        // std::string data2PrettyHex(const void* data, const std::size_t length);
        // /** Print HEX string from data; formatted uppercase with spaces between each byte pair, autosize */
        // template <typename T>
        // std::string data2PrettyHex(const T &data) { return data2PrettyHex(&data, sizeof(T)); }

        // template <std::size_t rowSize = 8, bool showASCII = true>
        // std::string dumpHex(const void* data, std::size_t len) { KittyUtils::HexDump<rowSize, showASCII>(data, len); }
    } // namespace hex
} // namespace fatigue