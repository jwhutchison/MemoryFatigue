#include <algorithm>
#include "utils.hpp"

namespace fatigue {

    namespace string {
        std::string toUpper(const std::string &str)
        {
            std::string out = std::string(str);
            std::transform(out.cbegin(), out.cend(), out.begin(), ::toupper);
            return out;
        }

        std::string toLower(const std::string &str)
        {
            std::string out = std::string(str);
            std::transform(out.cbegin(), out.cend(), out.begin(), ::tolower);
            return out;
        }

        std::string trim(std::string &str)
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
    } // namespace string


    namespace hex {
        std::string toHex(const void* data, const std::size_t length)
        {
            const unsigned char* bytes = static_cast<unsigned char const*>(data);
            const char* hexmap = "0123456789ABCDEF";

            std::string hex;
            hex.reserve(length * 2);
            for (std::size_t i = 0; i < length; ++i) {
                hex.push_back(hexmap[(bytes[i] & 0xF0) >> 4]); // high nibble
                hex.push_back(hexmap[(bytes[i] & 0x0F)]);      // low nibble
            }
            return hex;
        }


    } // namespace hex
    // std::string data2Hex(const void* data, const std::size_t length)
    // {
    //     return KittyUtils::data2Hex(data, length);
    // }

    // std::string data2PrettyHex(const void* data, const std::size_t length)
    // {
    //     std::string original = data2Hex(data, length);

    //     // Make an uppercase copy of the original string
    //     std::string result = string::toUpper(original);

    //     // Add spaces between each byte
    //     for (int i = 0; i < original.size(); i += 2) {
    //         result += original.substr(i, 2) + " ";
    //     }

    //     return result;
    // }
}
