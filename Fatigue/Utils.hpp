#pragma once

#include <algorithm>
#include <string>
#include "KittyUtils.hpp"

namespace Fatigue {

    template <typename T>
    std::string data2PrettyHex(const T &data)
    {
        std::string original = KittyUtils::data2Hex(data);
        std::transform(original.begin(), original.end(), original.begin(), ::toupper);
        std::string result;

        // Add spaces between each byte
        for (int i = 0; i < original.size(); i += 2) {
            result += original.substr(i, 2) + " ";
        }

        return result;
    }

    namespace String {
        std::string toUpper(const std::string &str);
        std::string toLower(const std::string &str);

        void trim(std::string &str);
        std::string toTrimmed(const std::string &str);
    }
}
