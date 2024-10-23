#pragma once

#include <cstdint>
#include <sys/types.h>
#include "proc.hpp"

/**
 * Dead simple memory reading and writing for Linux processes
 * Provides a Region class for working with processes
 */
namespace fatigue::mem {
    /**
     * @brief Memory region in a process
     * Represents a region of memory in a process, e.g. a mapped file, heap, stack, etc
     * with start and end addresses and optional name
     * Provides read and write methods for reading and writing memory in the region
     * Virtual base class for different memory access methods, use either io::Region or sys::Region
     */
    class IRegion {
    protected:
        std::string m_name;
        pid_t m_pid{0};
        unsigned long long m_start{0};
        unsigned long long m_end{0};
    public:
        IRegion() = default;
        IRegion(pid_t pid, uintptr_t start, uintptr_t end)
            : m_pid(pid), m_start(start), m_end(end) {}
        IRegion(pid_t pid, uintptr_t start, uintptr_t end, const std::string& name)
            : m_pid(pid), m_start(start), m_end(end)
        {
            m_name = std::string(name); // make a copy
        }
        IRegion(const proc::Map& map)
            : IRegion(map.pid, map.start, map.end, map.pathname) {}
        ~IRegion() = default;

        inline const std::string& name() const { return m_name; }
        inline pid_t pid() const { return m_pid; }
        inline uintptr_t start() const { return m_start; }
        inline uintptr_t end() const { return m_end; }
        inline size_t size() const { return m_end - m_start; }

        inline bool isValid() const { return m_pid > 0 && m_start > 0 && m_end > 0; }
        inline bool contains(uintptr_t address) const { return address >= m_start && address < m_end; }

        virtual ssize_t read(uintptr_t offset, void* buffer, size_t size) const = 0;
        virtual ssize_t write(uintptr_t offset, const void* buffer, size_t size) const = 0;

        template <typename T> ssize_t read(uintptr_t offset, T* value) const;
        template <typename T> ssize_t write(uintptr_t offset, const T* value) const;
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

        /**
         * @brief Memory region in a process
         * Represents a region of memory in a process, e.g. a mapped file, heap, stack, etc
         * with start and end addresses and optional name
         * Provides read and write methods for reading and writing memory in the region
         * Uses process_vm_readv and process_vm_writev syscalls for memory access
         */
        class Region : public IRegion {
        public:
            Region(pid_t pid, uintptr_t start, uintptr_t end)
                : IRegion(pid, start, end) {}
            Region(pid_t pid, uintptr_t start, uintptr_t end, const std::string& name)
                : IRegion(pid, start, end, name) {}
            Region(const proc::Map& map)
                : IRegion(map) {}
            ~Region() = default;

            ssize_t read(uintptr_t offset, void* buffer, size_t size) const override;
            ssize_t write(uintptr_t offset, const void* buffer, size_t size) const override;

            template <typename T> ssize_t read(uintptr_t offset, T* value) const
            {
                return read(offset, value, sizeof(T));
            }
            template <typename T> ssize_t write(uintptr_t offset, const T* value) const
            {
                return write(offset, value, sizeof(T));
            }
        };
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
         * @brief Memory region in a process
         * Represents a region of memory in a process, e.g. a mapped file, heap, stack, etc
         * with start and end addresses and optional name
         * Provides read and write methods for reading and writing memory in the region
         * Uses /proc/[pid]/mem file IO for memory access with optional ptrace attach
         */
        class Region : public IRegion {
        protected:
            bool m_usePtrace{false};
            bool m_attached{false};
        public:
            Region(pid_t pid, uintptr_t start, uintptr_t end, bool usePtrace = false)
                : IRegion(pid, start, end), m_usePtrace(usePtrace) {}
            Region(pid_t pid, uintptr_t start, uintptr_t end, const std::string& name, bool usePtrace = false)
                : IRegion(pid, start, end, name), m_usePtrace(usePtrace) {}
            Region(const proc::Map& map, bool usePtrace = false)
                : IRegion(map), m_usePtrace(usePtrace) {}
            ~Region() = default;

            bool usePtrace() const { return m_usePtrace; }
            bool attached() const { return m_attached; }

            void attach();
            void detach();

            // TODO: Consider adding a smarter read/write that can use an existing fd
            //       we can keep a single fd open for the lifetime of the attachment
            //       and use it for several reads and writes, closing it on detach

            ssize_t read(uintptr_t offset, void* buffer, size_t size) const override;
            ssize_t write(uintptr_t offset, const void* buffer, size_t size) const override;

            template <typename T> ssize_t read(uintptr_t offset, T* value) const
            {
                return read(offset, value, sizeof(T));
            }
            template <typename T> ssize_t write(uintptr_t offset, const T* value) const
            {
                return write(offset, value, sizeof(T));
            }
        };
    } // namespace io
}
