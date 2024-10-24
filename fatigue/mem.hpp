#pragma once

#include <cstdint>
#include <sys/types.h>
#include "proc.hpp"

/**
 * Dead simple memory reading and writing for Linux processes
 * Provides a Region class for working with processes
 */
namespace fatigue::mem {
    /** Memory access method, used by Region to choose between syscalls and file IO */
    enum class AccessMethod {
        SYS,
        IO
    };

    /**
     * Read and write process memory using syscalls
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

        /** Read process memory from /proc/[pid]/mem into known type */
        template <typename T>
        ssize_t read(pid_t pid, uintptr_t address, T* value, bool usePtrace = false)
        {
            return read(pid, address, value, sizeof(T), usePtrace);
        }
        /** Write process memory from /proc/[pid]/mem from known type */
        template <typename T>
        ssize_t write(pid_t pid, uintptr_t address, const T* value, bool usePtrace = false)
        {
            return write(pid, address, value, sizeof(T), usePtrace);
        }

        /**
         * Batch read and write process memory using /proc/[pid]/mem file IO
         * Optionally attach and detach using ptrace
         * Uses a single file descriptor for multiple reads and writes
         */
        class Batch {
        protected:
            pid_t m_pid{0};
            bool m_usePtrace{false};
            bool m_attached{false};
            int m_fd{-1};
        public:
            Batch() = default;
            ~Batch() { stop(); }

            inline pid_t pid() const { return m_pid; }
            inline bool usePtrace() const { return m_usePtrace; }
            inline bool attached() const { return m_attached; }
            inline int fd() const { return m_fd; }

            int start(pid_t pid, bool usePtrace = false);
            void stop();

            ssize_t read(uintptr_t address, void* buffer, size_t size);
            ssize_t write(uintptr_t address, const void* buffer, size_t size);

            template <typename T>
            ssize_t read(uintptr_t offset, T* value) const
            {
                return read(offset, value, sizeof(T));
            }
            template <typename T>
            ssize_t write(uintptr_t offset, const T* value) const
            {
                return write(offset, value, sizeof(T));
            }
        };
    } // namespace io
}
