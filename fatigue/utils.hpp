#pragma once

#ifndef FATIGUE_COLOR
#define FATIGUE_COLOR true
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

        std::ostream &operator<<(std::ostream &os, Color code);

        /** @brief Colorize a string with ANSI color codes */
        std::string colorize(Color code, std::string const &str);
    } // namespace color

} // namespace fatigue