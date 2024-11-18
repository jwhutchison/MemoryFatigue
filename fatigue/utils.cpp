#include "utils.hpp"

namespace fatigue {
    namespace search {
        Pattern parsePattern(std::string_view hex)
        {
            std::vector<std::string> in = hex::split(hex);

            if (in.empty())
                return Pattern();

            Pattern out;
            out.bytes.reserve(in.size());

            for (auto& byte : in) {
                if (byte == "??") {
                    out.bytes.push_back(0);
                    out.mask += "?";
                } else {
                    out.bytes.push_back(static_cast<uint8_t>(std::stoi(byte, nullptr, 16)));
                    out.mask += ".";
                }
            }

            return out;
        }


        std::vector<uintptr_t> search(const void* haystack, size_t haystackSize,
                                      const void* needle, size_t needleSize,
                                      std::string_view mask, bool first)
        {
            std::vector<uintptr_t> found = {};

            if (!mask.empty() && mask.at(0) == '?')
                throw std::invalid_argument("Mask should not start with a wildcard");

            if (!haystack || !needle || haystackSize < 1 || needleSize < 1)
                return found;

            const uint8_t* h = static_cast<const uint8_t*>(haystack);
            const uint8_t* n = static_cast<const uint8_t*>(needle);

            for (uintptr_t i = 0; i < haystackSize; /* increment in loop */)
            {
                // While comparing the needle against the haystack, we look at the next few haystack bytes,
                // so we might as well consider the next possible match while we are looking at this one.
                // If we see the first byte of the needle, we can skip ahead to that byte in the next iteration,
                // and if we don't see the first byte (match or break), we can skip ahead as far as we've looked
                // 0 means we haven't seen it; >0 means we have, so don't change it again
                int inc = 0;

                // Check each byte in the needle against the haystack starting at i
                for (uintptr_t j = 0; j < needleSize; j++)
                {
                    // If the needle byte is a mask wildcard, skip checks
                    bool masked = mask.length() > j && mask.at(j) == '?';

                    // If current haystack byte is the same as the first needle byte, set inc for next iteration
                    if (!masked && j > 0 && h[i + j] == n[0]) {
                        // Only set the inc if we haven't seen it yet
                        if (inc == 0) inc = j;
                    }

                    // if the current needle byte is not the same as the haystack byte, range is not a match
                    if (!masked && n[j] != h[i + j])
                    {
                        // Never saw the first byte of the needle, so skip ahead to the next byte
                        if (inc == 0) inc = j + 1;

                        break;
                    }

                    // If we've reached the end of the needle, we have a match
                    if (j == needleSize - 1)
                    {
                        // Never saw the first byte of the needle, so skip ahead to the next byte
                        if (inc == 0) inc = j + 1;

                        // Add the offset of the match to the result
                        found.push_back(i);

                        // If we only want the first match, return now
                        if (first)
                            return found;
                    }
                }

                // Skip ahead to the next instance of the first byte if found in the needle, or the next byte
                i += inc > 0 ? inc : 1;
            }

            return found;
        }
    } // namespace search

    namespace string {
        std::string toUpper(std::string_view str)
        {
            std::string out = std::string(str);
            std::transform(out.cbegin(), out.cend(), out.begin(), ::toupper);
            return out;
        }

        std::string toLower(std::string_view str)
        {
            std::string out = std::string(str);
            std::transform(out.cbegin(), out.cend(), out.begin(), ::tolower);
            return out;
        }

        std::string trim(std::string_view str)
        {
            std::string out = std::string(str);

            out.erase(out.begin(), std::find_if(out.begin(), out.end(), [](char ch) {
                return std::isprint(ch) && !std::isspace(ch);
            }));
            out.erase(std::find_if(out.rbegin(), out.rend(), [](char ch) {
                return std::isprint(ch) && !std::isspace(ch);
            }).base(), out.end());

            return out;
        }

        std::string compact(std::string_view str)
        {
            std::string out = string::trim(str);
            out.erase(std::remove_if(out.begin(), out.end(), isspace), out.end());
            return out;
        }
    } // namespace string


    namespace hex {
        bool isValid(std::string_view hex, bool strict)
        {
            // Remove spaces (so that count is accurate)
            std::string in = string::compact(hex);

            // Must be an even number of characters
            if (in.empty() && in.size() % 2 != 0) return false;

            // Must not contain wildcards if strict
            if (strict && in.find('?') != std::string::npos) return false;

            // Must contain only valid hex characters
            std::string allowed = "0123456789ABCDEFabcdef?";
            if (in.find_first_not_of(allowed) != std::string::npos) return false;

            // Do any necessary checks on pairs of characters
            for (size_t i = 0; i < in.length(); i += 2) {
                std::string pair = in.substr(i, 2);
                // ? must be in pairs, so single ? is invalid
                if (std::count(pair.begin(), pair.end(), '?') == 1)
                    return false;
            }

            return true;
        }

        std::vector<std::string> split(std::string_view hex)
        {
            if (!isValid(hex))
                return {};

            // remove spaces
            std::string in = string::compact(hex);

            // Split into pairs of characters
            std::vector<std::string> out;
            out.reserve(in.size() / 2);

            for (size_t i = 0; i < in.length(); i += 2) {
                out.push_back(in.substr(i, 2));
            }

            return out;
        }

        std::string prettify(std::string_view hex)
        {
            if (!isValid(hex))
                return "";

            std::string in = string::toUpper(hex);

            std::string out;
            out.reserve(in.size() + in.size() / 2);
            // Add spaces between each byte
            for (std::size_t i = 0; i < in.size(); i += 2) {
                out += in.substr(i, 2) + " ";
            }

            return string::trim(out);
        }

        std::string toAscii(const void* data, const std::size_t length)
        {
            if (!data || length == 0)
                return "";

            const uint8_t* bytes = static_cast<uint8_t const*>(data);

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

            const uint8_t* bytes = static_cast<uint8_t const*>(data);
            const char* hexmap = "0123456789ABCDEF";

            std::string hex;
            hex.reserve(length * 2);
            for (std::size_t i = 0; i < length; ++i) {
                hex.push_back(hexmap[(bytes[i] & 0xF0) >> 4]); // high nibble
                hex.push_back(hexmap[(bytes[i] & 0x0F)]);      // low nibble
            }
            return hex;
        }

        std::vector<uint8_t> parse(std::string_view hex)
        {
            // Must be actual hex, no wildcards
            if (!isValid(hex, true))
                return {};

            // Convert each byte pair to a byte data
            std::vector<std::string> in = split(hex);

            std::vector<uint8_t> out;
            out.reserve(in.size());
            for (auto& byte : in)
            {
                out.push_back(static_cast<uint8_t>(std::stoi(byte, nullptr, 16)));
            }

            return out;
        }

        std::vector<uint8_t> parse(const void* data, const std::size_t length)
        {
            if (!data || length == 0)
                return {};

            const uint8_t* bytes = static_cast<uint8_t const*>(data);

            std::vector<uint8_t> out;
            out.reserve(length);
            for (std::size_t i = 0; i < length; ++i) {
                out.push_back(bytes[i]);
            }

            return out;
        }

        std::string dump(const void* data, std::size_t length, unsigned long long startAddress, std::size_t rowSize, bool showASCII)
        {
            if (!data || length == 0 || rowSize == 0)
                return "";

            const uint8_t *bytes = static_cast<const uint8_t *>(data);

            std::string out;

            for (size_t i = 0; i < length; i += rowSize)
            {
                out += std::format(
                    "{:#08x}: {:{}s} {}\n",
                    // offset
                    i + startAddress,
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
