#include "Utils.hpp"

namespace Fatigue {

    std::string String::toUpper(const std::string &str)
    {
        std::string copy = std::string(str);
        std::transform(copy.begin(), copy.end(), copy.begin(), ::toupper);
        return copy;
    }

    std::string String::toLower(const std::string &str)
    {
        std::string copy = std::string(str);
        std::transform(copy.begin(), copy.end(), copy.begin(), ::tolower);
        return copy;
    }

    void String::trim(std::string &str)
    {
        str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), str.end());
    }

    std::string String::toTrimmed(const std::string &str)
    {
        std::string copy = std::string(str);
        trim(copy);
        return copy;
    }
}
