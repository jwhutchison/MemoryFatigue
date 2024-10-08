#pragma once

#include <cstdint>
#include "KittyMemoryEx.hpp"
#include "KittyMemoryMgr.hpp"

namespace fatigue {
    // Proxy to KittyMemoryEx namespace
    using namespace KittyMemoryEx;

    /**
     * Get process name by PID from /proc/[pid]/status
     * Typically, this is just the executable name without the path.
     * For example, Windows processes would be just the name of the exe file, like "explorer.exe"
     */
    std::string getProcessStatusName(pid_t pid);

    /**
     * Get process cmline by PID from /proc/[pid]/cmdline
     * Alias for KittyMemoryEx::getProcessName
     */
    constexpr auto getProcessCmdline = KittyMemoryEx::getProcessName;

    // KittyMemoryEx::getProcessID actually checks == /proc/[pid]/cmdline, so add some more flexible helpers.
    pid_t getProcessIDByComparator(const std::string &processName, bool (*comparator)(const std::string &, const int));
    pid_t getProcessIDByStatusName(const std::string &processName);
    pid_t getProcessIDByCmdlineEndsWith(const std::string &processName);
    pid_t getProcessIDByCmdlineContains(const std::string &processName);


    /**
     * Memory manager for Fatigue
     * Extends KittyMemoryMgr, adding PE (Portable Executable) format for Windows features
     */
    class Fatigue : public KittyMemoryMgr {
    public:
        Fatigue() : KittyMemoryMgr() {}

        /**
         * Initialize memory manager
         * @param pid remote process ID
         * @param eMemOp: Memory read & write operation type [ EK_MEM_OP_SYSCALL / EK_MEM_OP_IO ]
         * @param initMemPatch: initialize MmeoryPatch & MemoryBackup instances, pass true if you want to use memPatch & memBackup
         */
        bool initialize(pid_t pid, EKittyMemOP eMemOp, bool initMemPatch);

        inline std::string processCmdline() const { return processName(); }
        inline std::string processStatusName() const { return m_processStatusName; }
    private:
        std::string m_processStatusName;
    };
}
