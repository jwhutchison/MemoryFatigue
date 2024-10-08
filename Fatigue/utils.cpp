#include "utils.hpp"

namespace fatigue {

    std::string string::toUpper(const std::string &str)
    {
        std::string out = std::string(str);
        std::transform(out.cbegin(), out.cend(), out.begin(), ::toupper);
        return out;
    }

    std::string string::toLower(const std::string &str)
    {
        std::string out = std::string(str);
        std::transform(out.cbegin(), out.cend(), out.begin(), ::tolower);
        return out;
    }

    std::string string::trim(std::string &str)
    {
        std::string out = std::string(str);

        out.erase(out.begin(), std::find_if(out.begin(), out.end(), [](unsigned char ch) {
            return std::isprint(ch) && !std::isspace(ch);
        }));
        out.erase(std::find_if(out.rbegin(), out.rend(), [](unsigned char ch) {
            return std::isprint(ch) && !std::isspace(ch);
        }).base(), out.end());

        return out;
    }

    std::string data2Hex(const void* data, const std::size_t length)
    {
        return KittyUtils::data2Hex(data, length);
    }

    std::string data2PrettyHex(const void* data, const std::size_t length)
    {
        std::string original = data2Hex(data, length);

        // Make an uppercase copy of the original string
        std::string result = string::toUpper(original);

        // Add spaces between each byte
        for (int i = 0; i < original.size(); i += 2) {
            result += original.substr(i, 2) + " ";
        }

        return result;
    }
}
