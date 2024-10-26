#pragma once

#include <string>
#include "mem.hpp"

#ifndef DEFAULT_MEMORY_ACCESS_METHOD
#define DEFAULT_MEMORY_ACCESS_METHOD fatigue::mem::AccessMethod::SYS
#endif

namespace fatigue {
    /**
     * @brief Memory region in a process
     * Represents a region of memory in a process, e.g. a mapped file, heap, stack, etc
     * with start and end addresses and optional name
     * Provides read and write methods for reading and writing memory in the region
     * Virtual base class for different memory access methods, use either io::Region or sys::Region
     */
    class Region {
    public:
        mem::AccessMethod method{mem::AccessMethod::SYS};

        std::string name;
        pid_t pid{0};
        unsigned long long start{0};
        unsigned long long end{0};

        Region() = default;
        Region(pid_t pid, uintptr_t start, uintptr_t end)
            : pid(pid), start(start), end(end) {}
        Region(pid_t pid, uintptr_t start, uintptr_t end, const std::string& name)
            : pid(pid), start(start), end(end), name(name) {}
        ~Region() = default;

        inline size_t size() const { return end - start; }
        inline bool isValid() const { return pid > 0 && start > 0 && end > 0 && end > start; }
        inline bool contains(uintptr_t address) const { return address >= start && address < end; }

        // Read and write

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

        // Pattern scanning
        // TODO: Need PE sections first, eedjit
        // std::vector<uintptr_t> findBytesAll(const char* bytes, const std::string& mask) const;
        // uintptr_t findBytesFirst(const char* bytes, const std::string& mask) const;
        // std::vector<uintptr_t> findHexAll(std::string hex, const std::string& mask) const;
        // uintptr_t findHexFirst(std::string hex, const std::string& mask) const;
        // std::vector<uintptr_t> findPatternAll(const std::string& pattern) const;
        // uintptr_t findPatternFirst(const std::string& pattern) const;
    };
} // namespace fatigue
