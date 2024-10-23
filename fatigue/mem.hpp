#pragma once

#include <cstdint>
#include <sys/types.h>

/**
 * Dead simple memory reading and writing for Linux processes
 */
namespace fatigue::mem {
    /**
     * Read and write process memory using /proc/[pid]/mem file IO
     */
    namespace io {
        /** Attach to a process using ptrace and wait */
        void attach(pid_t pid);
        /** Detach from a process using ptrace */
        void detach(pid_t pid);
        /** Read process memory from /proc/[pid]/mem, optionally using ptrace attach */
        ssize_t read(pid_t pid, uintptr_t address, void* buffer, size_t size, bool usePtrace = false);
        /** Write process memory from /proc/[pid]/mem, optionally using ptrace attach */
        ssize_t write(pid_t pid, uintptr_t address, const void* buffer, size_t size, bool usePtrace = false);
    } // namespace io

    /**
     * Read and write process memory using syscalls
     */
    namespace sys {
        /** Read process memory using process_vm_readv */
        ssize_t read(pid_t pid, uintptr_t address, void* buffer, size_t size);
        /** Write process memory using process_vm_writev */
        ssize_t write(pid_t pid, uintptr_t address, const void* buffer, size_t size);
    } // namespace sys
}
