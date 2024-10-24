#pragma once

#ifndef DEFAULT_MEMORY_ACCESS_METHOD
#define DEFAULT_MEMORY_ACCESS_METHOD fatigue::mem::AccessMethod::SYS
#endif

#include "mem.hpp"

using namespace fatigue::mem;

namespace fatigue {
    // TODO: HERE make this a real class, use an enum to select the method

    /**
     * @brief Memory region in a process
     * Represents a region of memory in a process, e.g. a mapped file, heap, stack, etc
     * with start and end addresses and optional name
     * Provides read and write methods for reading and writing memory in the region
     * Virtual base class for different memory access methods, use either io::Region or sys::Region
     */
    class Region {
    protected:
        std::string m_name;
        pid_t m_pid{0};
        unsigned long long m_start{0};
        unsigned long long m_end{0};
    public:
        Region() = default;
        Region(pid_t pid, uintptr_t start, uintptr_t end)
            : m_pid(pid), m_start(start), m_end(end) {}
        Region(pid_t pid, uintptr_t start, uintptr_t end, const std::string& name)
            : m_pid(pid), m_start(start), m_end(end), m_name(name) {}
        Region(const proc::Map& map)
            : Region(map.pid, map.start, map.end, map.pathname) {}
        ~Region() = default;

        AccessMethod method{DEFAULT_MEMORY_ACCESS_METHOD};

        inline const std::string& name() const { return m_name; }
        inline pid_t pid() const { return m_pid; }
        inline uintptr_t start() const { return m_start; }
        inline uintptr_t end() const { return m_end; }
        inline size_t size() const { return m_end - m_start; }

        inline bool isValid() const { return m_pid > 0 && m_start > 0 && m_end > 0; }
        inline bool contains(uintptr_t address) const { return address >= m_start && address < m_end; }

        ssize_t read(uintptr_t offset, void* buffer, size_t size) const;
        ssize_t write(uintptr_t offset, const void* buffer, size_t size) const;

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
} // namespace fatigue
