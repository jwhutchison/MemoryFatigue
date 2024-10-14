#pragma once

#include <cstdint>
#include "KittyMemoryEx.hpp"
#include "KittyMemoryMgr.hpp"
#include "PE.hpp"

namespace fatigue {
    // Proxy the KittyMemoryEx namespace
    using namespace KittyMemoryEx;

    /**
     * Get process name by PID from /proc/[pid]/status
     * Typically, this is just the executable name without the path.
     * For example, Windows processes would be just the name of the exe file, like "explorer.exe"
     */
    std::string getStatusName(pid_t pid);

    /**
     * Get process cmline by PID from /proc/[pid]/cmdline
     * Alias for KittyMemoryEx::getProcessName
     */
    constexpr auto getProcessCmdline = KittyMemoryEx::getProcessName;

    /**
     * Get a process ID by strict matching to /proc/[pid]/cmdline
     * Proxy for KittyMemoryEx::getProcessID (to allow overloading)
     */
    inline pid_t getProcessID(const std::string &processName) { return KittyMemoryEx::getProcessID(processName); }

    // KittyMemoryEx::getProcessID actually checks == /proc/[pid]/cmdline, so add some more flexible helpers.

    /**
     * Get a process ID by custom matching function, e.g. by cmdline or status name
     * @param processName: name to match
     * @param comparator: function to match the desired process name and the PID to check against
     */
    pid_t getProcessID(const std::string &processName, bool (*comparator)(const std::string &, const int));
    /**
     * Get a process ID by strict matching /proc/[pid]/status Name
     */
    pid_t getProcessIDByStatusName(const std::string &processName);
    /**
     * Get a process ID by matching /proc/[pid]/cmdline
     * Matches any substring from end of cmdline, so more specificity is better to prvent false positives
     * For strict matching to /proc/[pid]/cmdline, use getProcessID()
     */
    pid_t getProcessIDByCmdlineEndsWith(const std::string &processName);
    /**
     * Get a process ID by matching /proc/[pid]/cmdline
     * Matches any substring from anywhere in cmdline, so more specificity is better to prvent false positives
     * For strict matching to /proc/[pid]/cmdline, use getProcessID()
     */
    pid_t getProcessIDByCmdlineContains(const std::string &processName);

    /**
     * Memory manager for Fatigue
     * Extends KittyMemoryMgr, adding PE (Portable Executable) format for Windows features and some convenience
     */
    class Fatigue : public KittyMemoryMgr {
    public:
        pe::PeScannerMgr peScanner{nullptr};

        Fatigue() : KittyMemoryMgr() {}
        Fatigue(pid_t pid, EKittyMemOP eMemOp, bool initMemPatch) : KittyMemoryMgr() {
            initialize(pid, eMemOp, initMemPatch);
        }

        /** Simple factory method to attach to a process with a known pid */
        static Fatigue* attach(pid_t pid, EKittyMemOP eMemOp = EK_MEM_OP_IO, bool initMemPatch = false);
        /** Simple factory method to attach to a process with a running process cmdline */
        static Fatigue* attach(const std::string &processName, EKittyMemOP eMemOp = EK_MEM_OP_IO, bool initMemPatch = false);
        /** Simple factory method to attach to a process with a running process status name and a custom function to get the process ID */
        static Fatigue* attach(
            const std::string &processName,
            std::function<pid_t(const std::string&)> getPidFn,
            EKittyMemOP eMemOp = EK_MEM_OP_IO,
            bool initMemPatch = false);

        // TODO: Add wait and attach for

        /**
         * Initialize memory manager
         * @param pid remote process ID
         * @param eMemOp: Memory read & write operation type [ EK_MEM_OP_SYSCALL / EK_MEM_OP_IO ]
         * @param initMemPatch: initialize MmeoryPatch & MemoryBackup instances, pass true if you want to use memPatch & memBackup
         */
        bool initialize(pid_t pid, EKittyMemOP eMemOp = EK_MEM_OP_IO, bool initMemPatch = false);

        inline pid_t getProcessID() const { return processID(); }
        inline std::string getCmdline() const { return processName(); }
        inline std::string getStatusName() const { return m_statusName; }

        int getStatusInteger(const std::string &var) const { return KittyMemoryEx::getStatusInteger(processID(), var); }

        bool isReady() const { return isMemValid(); }

        /**
         * Validate PE
         * @param base: start address
         */
        bool isValidPe(uintptr_t base) const;

        /**
         * Find in-memory loaded ELF with name
         */
        pe::SectionScanner* getPeSection(const std::string &sectionName);

    private:
        std::string m_statusName;
    };
}
