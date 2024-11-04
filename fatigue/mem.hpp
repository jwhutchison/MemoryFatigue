#pragma once

#include <cstdint>
#include <sys/types.h>

/**
 * Dead simple memory reading and writing for Linux processes
 * Provides a Region class for working with processes
 */
namespace fatigue::mem {
    /** Memory access method, used by Region to choose between syscalls and file IO */
    enum class AccessMethod {
        SYS,
        IO,
        PTRACE
    };

    void setAccessMethod(AccessMethod method);
    AccessMethod getAccessMethod();

    /**
     * Read and write process memory using syscalls
     * Fast, but will not work to patch memory of another process (see io::write or trace::write)
     */
    namespace sys {
        /** Read process memory using process_vm_readv */
        ssize_t read(pid_t pid, uintptr_t address, void* buffer, size_t size);
        /** Write process memory using process_vm_writev */
        ssize_t write(pid_t pid, uintptr_t address, const void* buffer, size_t size);

        /** Read process memory using process_vm_readv into known type */
        template <typename T>
        ssize_t read(pid_t pid, uintptr_t address, T* value)
        {
            return read(pid, address, value, sizeof(T));
        }
        /** Write process memory using process_vm_writev from known type */
        template <typename T>
        ssize_t write(pid_t pid, uintptr_t address, const T* value)
        {
            return write(pid, address, value, sizeof(T));
        }
    } // namespace sys

    /**
     * Read and write process memory using /proc/[pid]/mem file IO
     * Important, if you intend to write to memory of another process, you will need to attach first,
     * use io::write or trace::write (sys::write will not work!), and then detach.
     */
    namespace io {
        /** Read process memory from /proc/[pid]/mem */
        ssize_t read(pid_t pid, uintptr_t address, void* buffer, size_t size);
        /** Write process memory from /proc/[pid]/mem */
        ssize_t write(pid_t pid, uintptr_t address, const void* buffer, size_t size);

        /** Read process memory from /proc/[pid]/mem into known type */
        template <typename T>
        ssize_t read(pid_t pid, uintptr_t address, T* value)
        {
            return read(pid, address, value, sizeof(T));
        }
        /** Write process memory from /proc/[pid]/mem from known type */
        template <typename T>
        ssize_t write(pid_t pid, uintptr_t address, const T* value)
        {
            return write(pid, address, value, sizeof(T));
        }
    } // namespace io

    /**
     * Read and write process memory using ptrace
     * Slow, but should work everywhere PTRACE is available
     * Important, if you intend to write to memory of another process, you will need to attach first,
     * use io::write or trace::write (sys::write will not work!), and then detach.
     */
    namespace trace {
        /** Read process memory using ptrace */
        size_t read(pid_t pid, uintptr_t address, void* buffer, size_t size);
        /** Write process memory using ptrace */
        size_t write(pid_t pid, uintptr_t address, const void* buffer, size_t size);

        /** Read process memory using ptrace into known type */
        template <typename T>
        size_t read(pid_t pid, uintptr_t address, T* value)
        {
            return read(pid, address, value, sizeof(T));
        }
        /** Write process memory using ptrace from known type */
        template <typename T>
        size_t write(pid_t pid, uintptr_t address, const T* value)
        {
            return write(pid, address, value, sizeof(T));
        }
    } // namespace ptrace
}
