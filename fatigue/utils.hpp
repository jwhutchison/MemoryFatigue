#pragma once

#include <algorithm>
#include <string>
#include "KittyUtils.hpp"

namespace fatigue {
    namespace string {
        /** Return an uppercase copy of a string */
        std::string toUpper(const std::string &str);
        /** Return a lowercase copy of a string */
        std::string toLower(const std::string &str);
        /** Copy a string and trim leading and trailing spaces and unprintable characters */
        std::string trim(std::string &str);

        // Alias useful string functions from KittyUtils

        constexpr auto startsWith = KittyUtils::String::StartsWith;
        constexpr auto endsWith = KittyUtils::String::EndsWith;
        constexpr auto contains = KittyUtils::String::Contains;
        constexpr auto validateHex = KittyUtils::String::ValidateHex;
        constexpr auto fmt = KittyUtils::String::Fmt;
    }

    // Alias some very useful functions from KittyUtils

    /** Convert a chunk of data to its hexadecimal representation */
    std::string data2Hex(const void* data, const std::size_t dataLength);
    /** Convert a chunk of data to its hexadecimal representation, autosize */
    template <typename T>
    std::string data2Hex(const T &data) { return KittyUtils::data2Hex<T>(data); }
    /** Convert a hexadecimal string to a chunk of data */
    constexpr auto dataFromHex = KittyUtils::dataFromHex;

    /** Print HEX string from data; formatted uppercase with spaces between each byte pair */
    std::string data2PrettyHex(const void* data, const std::size_t length);
    /** Print HEX string from data; formatted uppercase with spaces between each byte pair, autosize */
    template <typename T>
    std::string data2PrettyHex(const T &data) { return data2PrettyHex(&data, sizeof(T)); }

    template <std::size_t rowSize = 8, bool showASCII = true>
    std::string dumpHex(const void* data, std::size_t len) { KittyUtils::HexDump<rowSize, showASCII>(data, len); }
}
