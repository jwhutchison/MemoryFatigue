#pragma once

#include <cstdint>
#include "KittyMemoryEx.hpp"

namespace Fatigue {
    // Proxy to KittyMemoryEx namespace
    using namespace KittyMemoryEx;

    /**
     * Get process name by PID from /proc/[pid]/status
     * Typically, this is just the executable name without the path.
     * For example, Windows processes would be just the name of the exe file, like "explorer.exe"
     */
    std::string getProcessStatusName(pid_t pid);

    // KittyMemoryEx::getProcessID actually checks == /proc/[pid]/cmdline, so add some more flexible helpers.
    pid_t getProcessIDByComparator(const std::string &processName, bool (*comparator)(const std::string &, const int));
    pid_t getProcessIDByStatusName(const std::string &processName);
    pid_t getProcessIDByCmdlineEndsWith(const std::string &processName);
    pid_t getProcessIDByCmdlineContains(const std::string &processName);
}