#pragma once

#include <format>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include "Region.hpp"

namespace fatigue::proc {
    class Map : public Region
    {
    public:
        std::string perms;
        unsigned long long offset{0};
        std::string dev;
        unsigned long inode{0};

        Map() = default;
        Map(pid_t pid, uintptr_t start, uintptr_t end, const std::string& perms, unsigned long long offset, const std::string& dev, unsigned long inode, const std::string& name)
            : Region(pid, start, end, name), perms(perms), offset(offset), dev(dev), inode(inode) {}
        ~Map() = default;

        inline bool isRead() const { return !perms.empty() && perms.at(0) == 'r'; }
        inline bool isWrite() const { return !perms.empty() && perms.at(1) == 'w'; }
        inline bool isExec() const { return !perms.empty() && perms.at(2) == 'x'; }
        inline bool isPrivate() const { return !perms.empty() && perms.at(3) == 'p'; }
        inline bool isShared() const { return !perms.empty() && perms.at(3) == 's'; }

        inline bool isAnonymous() const { return name.empty(); }
        inline bool isPsuedo() const { return !perms.empty() && name.at(0) == '['; }
        inline bool isFile() const { return !isAnonymous() && !isPsuedo(); }

        inline std::string toString()
        {
            return std::format("{:#x}-{:#x} {}{}{}{} {:#x} {} {} {}",
                               start, end,
                               isRead() ? 'r' : '-', isWrite() ? 'w' : '-', isExec() ? 'x' : '-', isPrivate() ? 'p' : 's',
                               offset, dev.c_str(), inode, name.c_str());
        }
    };

    /**
     * Get all status information from /proc/[pid]/status
     * Returns a map of key-value pairs as strings
     */
    std::map<std::string, std::string> getStatus(pid_t pid);

    /**
     * Get process name by PID from /proc/[pid]/status
     * Typically, this is just the executable name without the path.
     * For example, Windows processes would be just the name of the exe file,
     * like "explorer.exe"
     */
    std::string getStatusName(pid_t pid);

    /**
     * Get process cmdline by PID from /proc/[pid]/cmdline
     */
    std::string getCmdline(pid_t pid);

    /**
     * Get a process ID by custom matching function, e.g. by cmdline or status
     * name
     * @param processName: name to match
     * @param comparator: function to match the desired process name and the PID
     * to check against
     */
    pid_t getProcessId(const std::string& processName, std::function<bool(pid_t)> filter);

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

    /**
     * Wait for a process to appear using one of the getProcess* functions
     * optionally with a timeout
     */
    pid_t waitForProcess(std::function<pid_t()> get, u_int timeout = 0, u_int interval = 1);

    /**
     * Sleep for a number of milliseconds
     * Useful if the process needs a moment to start up
     */
    void wait(u_int ms = 1000);

    /**
     * Get all maps in /proc/[pid]/maps
     * Optionally, provide a custom comparator to filter maps, e.g. by pathname
     */
    std::vector<Map> getMaps(pid_t pid, std::function<bool(Map&)> filter = nullptr);

    /**
     * Get all maps in /proc/[pid]/maps that are valid and have a pathname
     */
    std::vector<Map> getValidMaps(pid_t pid);
    /**
     * Get all maps in /proc/[pid]/maps that are valid and have a pathname that contains the provided name
     */
    std::vector<Map> getMaps(pid_t pid, const std::string& name);
    /**
     * Get all maps in /proc/[pid]/maps that are valid and have a pathname that ends with the provided name
     */
    std::vector<Map> getMapsEndsWith(pid_t pid, const std::string& name);

    /**
     * Find the first map in /proc/[pid]/maps that has a pathname that contains the provided name
     */
    Map findMap(pid_t pid, const std::string& name);
    /**
     * Find the first map in /proc/[pid]/maps that has a pathname that ends with the provided name
     */
    Map findMapEndsWith(pid_t pid, const std::string& name);

    /**
     * Attach to a process using ptrace, allowing read and write access to memory
     * Should detach from the process when done, or it will be left suspended
     * You will need to be the owner of the process or have the CAP_SYS_PTRACE capability.
     */
    bool attach(pid_t pid);
    /**
     * Detach from a process using ptrace
     */
    bool detach(pid_t pid);
} // namespace fatigue
