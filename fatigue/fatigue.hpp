#pragma once

#include <string>

namespace fatigue {
    /**
     * Get process name by PID from /proc/[pid]/status
     * Typically, this is just the executable name without the path.
     * For example, Windows processes would be just the name of the exe file,
     * like "explorer.exe"
     */
    std::string getStatusName(pid_t pid);

    // TODO: Get other status stuff?

    /**
     * Get process cmdline by PID from /proc/[pid]/cmdline
     */
    std::string getCmdline(pid_t pid);

    // TODO: Additional helpers from KittyMemoryEx

    /**
     * Get a process ID by custom matching function, e.g. by cmdline or status
     * name
     * @param processName: name to match
     * @param comparator: function to match the desired process name and the PID
     * to check against
     */
    pid_t getProcessId(const std::string& processName,
                       bool (*comparator)(const std::string&, const int));

    /**
     * Get a process ID by strict matching to /proc/[pid]/cmdline
     */
    pid_t getProcessId(const std::string& cmdline);

    /**
     * Get a process ID by matching /proc/[pid]/cmdline
     * Matches any substring from end of cmdline, so more specificity is better
     * to prvent false positives For strict matching to /proc/[pid]/cmdline, use
     * getProcessId()
     */
    pid_t getProcessIdByCmdlineEndsWith(const std::string& cmdline);

    /**
     * Get a process ID by matching /proc/[pid]/cmdline
     * Matches any substring from anywhere in cmdline, so more specificity is
     * better to prvent false positives For strict matching to
     * /proc/[pid]/cmdline, use getProcessId()
     */
    pid_t getProcessIdByCmdlineContains(const std::string& cmdline);

    /**
     * Get a process ID by strict matching /proc/[pid]/status Name
     */
    pid_t getProcessIdByStatusName(const std::string& cmdline);
} // namespace fatigue