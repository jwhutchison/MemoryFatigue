#include "utils.hpp"

namespace fatigue::color {
    std::ostream &operator<<(std::ostream &os, Color code)
    {
        if (!FATIGUE_COLOR) return os;
        return os << "\033[" << static_cast<int>(code) << "m";
    }

    std::string colorize(Color code, std::string const &str)
    {
        if (!FATIGUE_COLOR) return str;
        return "\033[" + std::to_string(static_cast<int>(code)) + "m" + str + "\033[0m";
    }
} // namespace fatigue::color
