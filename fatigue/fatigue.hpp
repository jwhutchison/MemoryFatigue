#pragma once

#include <format>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace fatigue {
    /**
     * See KittyMemoryEx::ProcMap
     */
    class Map {
    public:
        pid_t pid;
        unsigned long long startAddress;
        unsigned long long endAddress;
        std::string perms;
        unsigned long long offset;
        std::string dev;
        unsigned long inode;
        std::string pathname;

        Map() : pid(0), startAddress(0), endAddress(0), offset(0), inode(0) {}

        inline size_t length() const { return endAddress - startAddress; }
        inline bool isReadable() const { return !perms.empty() && perms.at(0) == 'r'; }
        inline bool isWriteable() const { return !perms.empty() && perms.at(1) == 'w'; }
        inline bool isExecutable() const { return !perms.empty() && perms.at(2) == 'x'; }
        inline bool isPrivate() const { return !perms.empty() && perms.at(3) == 'p'; }
        inline bool isShared() const { return !perms.empty() && perms.at(3) == 's'; }

        inline bool isValid() const { return (pid && startAddress && endAddress && length()); }
        inline bool hasName() const { return !pathname.empty(); }

        inline bool contains(uintptr_t address) const { return address >= startAddress && address < endAddress; }

        inline std::string toString()
        {
            return std::format("{:#x}-{:#x} {}{}{}{} {:#x} {} {} {}",
                               startAddress, endAddress,
                               isReadable() ? 'r' : '-', isWriteable() ? 'w' : '-', isExecutable() ? 'x' : '-', isPrivate() ? 'p' : 's',
                               offset, dev.c_str(), inode, pathname.c_str());
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


    bool readMem(pid_t pid, uintptr_t address, void* buffer, size_t size);

} // namespace fatigue