#include <algorithm>
#include <iomanip>
#include <sstream>
#include "utils.hpp"

#include "log.hpp"

namespace fatigue {
    namespace search {
        std::vector<uintptr_t> search(const void* haystack, size_t haystackSize,
                                      const void* needle, size_t needleSize,
                                      std::string_view mask, bool first)
        {
            std::vector<uintptr_t> found = {};

            if (!mask.empty() && mask.at(0) == '?')
                throw std::invalid_argument("Mask should not start with a wildcard");

            if (!haystack || !needle || haystackSize < 1 || needleSize < 1)
                return found;

            const char* h = static_cast<const char*>(haystack);
            const char* n = static_cast<const char*>(needle);

            for (size_t i = 0; i < haystackSize;)
            {
                // While comparing the needle against the haystack, we look at the next few bytes,
                // so we might as well consider the next possible match while we are looking at this one.
                // If we see the first byte of the needle, we can skip ahead to that byte in the next iteration,
                // also if we don't see the first byte ever, we can skip ahead as far as we've looked
                int offsetAdjust = 0;

                // Check each byte in the needle against the haystack starting at i
                for (size_t j = 0; j < needleSize; j++)
                {
                    // If the needle byte is a mask wildcard, skip checks
                    bool masked = mask.length() > j && mask.at(j) == '?';

                    // If current haystack byte is the same as the first needle byte, set offset for next iteration
                    if (!masked && j > 0 && h[i + j] == n[0]) {
                        offsetAdjust = j;
                    }

                    // if the current needle byte is not the same as the haystack byte, range is not a match
                    if (!masked && n[j] != h[i + j])
                    {
                        // If offset is 0, it means we never saw the first byte of the needle, so we can start here
                        if (offsetAdjust == 0)
                            offsetAdjust = j;
                        break;
                    }

                    // If we've reached the end of the needle, we have a match
                    if (j == needleSize - 1)
                    {
                        found.push_back(reinterpret_cast<uintptr_t>(i));
                        // If we only want the first match, return now
                        if (first)
                            return found;
                    }
                }

                // Skip ahead to the next instance of the first byte if found in the needle, or the next byte
                i += offsetAdjust > 0 ? offsetAdjust : 1;
            }

            return found;
        }
    } // namespace search

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

        std::string compact(std::string &str)
        {
            std::string out = string::trim(str);
            out.erase(std::remove_if(out.begin(), out.end(), isspace), out.end());
            return out;
        }
    } // namespace string


    namespace hex {
        bool isValid(const std::string &hex)
        {
            return hex.size() > 0 && hex.size() % 2 == 0
                && hex.find_first_not_of("0123456789ABCDEFabcdef") == std::string::npos;
        }

        std::string prettify(const std::string &hex)
        {
            std::string in = string::toUpper(hex);

            std::string out;
            out.reserve(in.size() + in.size() / 2);

            // Add spaces between each byte
            for (int i = 0; i < in.size(); i += 2) {
                out += in.substr(i, 2) + " ";
            }

            return string::trim(out);
        }

        std::string toAscii(const void* data, const std::size_t length)
        {
            if (!data || length == 0)
                return "";

            const unsigned char* bytes = static_cast<unsigned char const*>(data);

            std::string out;
            out.reserve(length);
            // For each byte, if printable, add to result, else add '.'
            for (std::size_t i = 0; i < length; ++i) {
                char byte = static_cast<char>(bytes[i]);
                out += std::isprint(byte) ? byte : '.';
            }

            return out;
        }

        std::string toHex(const void* data, const std::size_t length)
        {
            if (!data || length == 0)
                return "";

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

        void parse(const std::string &hex, void* buffer)
        {
            if (!buffer || !isValid(hex))
                return;

            // remove spaces
            std::string pruned = std::string(hex);
            pruned.erase(remove_if(pruned.begin(), pruned.end(), isspace), pruned.end());

            // For each hex pair, parse two characters to a byte and store in buffer
            auto* bytes = reinterpret_cast<unsigned char*>(buffer);
            for (size_t i = 0; i < pruned.length(); i += 2)
            {
                std::string byteString = hex.substr(i, 2);
                bytes[i / 2] = static_cast<unsigned char>(std::stoi(byteString, nullptr, 16));
            }
        }

        std::string dump(const void* data, std::size_t length, std::size_t rowSize, bool showASCII)
        {
            if (!data || length == 0 || rowSize == 0)
                return "";

            const unsigned char *bytes = static_cast<const unsigned char *>(data);

            std::string out;

            for (size_t i = 0; i < length; i += rowSize)
            {
                out += std::format(
                    "{:#08x}: {:{}s} {:s}\n",
                    // offset
                    i,
                    // hex
                    toPrettyHex(&bytes[i], std::min(rowSize, length - i)),
                    // hex padding (sets width of field)
                    rowSize * 3,
                    // ascii
                    showASCII ? toAscii(&bytes[i], std::min(rowSize, length - i)) : ""
                );
            }

            return out;
        }
    } // namespace hex
}
