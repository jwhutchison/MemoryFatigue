#pragma once

#ifndef NO_COLOR
#define NO_COLOR false
#endif

#include <iostream>
#include <string>
#include <vector>

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

    namespace search {
        struct Pattern {
            std::vector<unsigned char> bytes;
            std::string mask;
        };

        /**
         * Convert a hex based pattern to a byte array and mask
         * example: "AA BB ?? CC" -> {0xAA, 0xBB, 0x00, 0xCC}, "..??.."
         */
        Pattern parsePattern(std::string_view hex);

        /**
         * Search for a byte pattern in a memory range
         * @param haystack Pointer to the memory range to search
         * @param haystackSize Size of the memory range to search
         * @param needle Pointer to the byte pattern to search for
         * @param needleSize Size of the byte pattern to search for
         * @param mask Optional mask for ignoring certain bytes using '?' for wildcards
         *             example: "..??.." will check bytes 1, 2, 5, 6, but bytes 3 and 4 will always match
         * @param first If true, stop searching after the first match
         */
        std::vector<uintptr_t> search(const void* haystack, size_t haystackSize,
                                      const void* needle, size_t needleSize,
                                      std::string_view mask = "", bool first = false);

        /**
         * Search for a byte pattern in a memory range using a Pattern struct
         * (mostly used for hex strings passed through parsePattern)
         * @param haystack Pointer to the memory range to search
         * @param haystackSize Size of the memory range to search
         * @param pattern Byte pattern to search for (@see parsePattern)
         * @param first If true, stop searching after the first match
         */
        inline std::vector<uintptr_t> search(const void* haystack, size_t haystackSize,
                                             const Pattern& pattern, bool first = false)
        {
            return search(haystack, haystackSize, pattern.bytes.data(), pattern.bytes.size(), pattern.mask, first);
        }

        /**
         * Search for a byte pattern in a memory range using a hexadecimal string
         * Spaces are ignored. Wildcard bytes are allowed using "??" (not '?')
         * @example "AA BB ?? CC" will search for 0xAA, 0xBB, any byte, 0xCC
         * @param haystack Pointer to the memory range to search
         * @param haystackSize Size of the memory range to search
         * @param hex Hexadecimal string pattern to search for
         * @param first If true, stop searching after the first match
         */
        inline std::vector<uintptr_t> search(const void* haystack, size_t haystackSize,
                                             std::string_view hex, bool first = false)
        {
            return search(haystack, haystackSize, parsePattern(hex), first);
        }

    } // namespace search

    namespace string {
        /** Return an uppercase copy of a string */
        std::string toUpper(std::string_view str);
        /** Return a lowercase copy of a string */
        std::string toLower(std::string_view str);
        /** Copy a string and trim leading and trailing spaces and unprintable characters */
        std::string trim(std::string_view str);
        /** Copy a string and remove all spaces */
        std::string compact(std::string_view str);
    }

    namespace hex {
        /**
         * Check if a string is a valid hexadecimal string
         * @param hex The string to check
         * @param strict If true, only allow valid hex characters, otherwise '?' is also allowed
         */
        bool isValid(std::string_view hex, bool strict = false);
        /** Split a hexadecimal string into a vector of byte pairs */
        std::vector<std::string> split(std::string_view hex);
        /** Prettify a hexadecimal string with spaces between each byte pair */
        std::string prettify(std::string_view hex);
        /** Convert a hexadecimal string to its ASCII representation */
        std::string toAscii(const void* data, const std::size_t length);

        /** Convert a chunk of data to its hexadecimal representation */
        std::string toHex(const void* data, const std::size_t length);
        /** Convert a chunk of data to its hexadecimal representation, autosize */
        template <typename T>
        std::string toHex(const T &data) { return toHex(data, sizeof(T)); }

        /** Convert a hexadecimal string to a chunk of data */
        std::vector<unsigned char> parse(std::string_view hex);

        /** Print HEX string from data; formatted uppercase with spaces between each byte pair */
        inline std::string toPrettyHex(const void* data, const std::size_t length) { return prettify(toHex(data, length)); }
        /** Print HEX string from data; formatted uppercase with spaces between each byte pair, autosize */
        template <typename T>
        std::string toPrettyHex(const T &data) { return toPrettyHex(&data, sizeof(T)); }

        /** Print HEX dump from data; formatted with ASCII representation */
        std::string dump(const void* data, std::size_t length, std::size_t rowSize = 16, bool showASCII = true);
    } // namespace hex
} // namespace fatigue