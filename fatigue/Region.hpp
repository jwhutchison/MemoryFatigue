#pragma once

#include <string>
#include <vector>
#include "mem.hpp"

#ifndef DEFAULT_MEMORY_ACCESS_METHOD
#define DEFAULT_MEMORY_ACCESS_METHOD fatigue::mem::AccessMethod::SYS
#endif

using namespace fatigue::mem;

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
        AccessMethod method;

        std::string name;
        pid_t pid{0};
        unsigned long long start{0};
        unsigned long long end{0};

        Region() : method(getAccessMethod()) {};
        Region(pid_t pid, uintptr_t start, uintptr_t end)
            : method(getAccessMethod()), pid(pid), start(start), end(end) {}
        Region(pid_t pid, uintptr_t start, uintptr_t end, const std::string& name)
            : method(getAccessMethod()), pid(pid), start(start), end(end), name(name) {}
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
        std::vector<uintptr_t> find(const void* pattern, size_t patternSize, const std::string& mask, bool first = false) const;
        inline uintptr_t findFirst(const void* pattern, size_t patternSize, const std::string& mask) const
        {
            auto found = find(pattern, patternSize, mask, true);
            return found.empty() ? 0 : found.front();
        }

        std::vector<uintptr_t> find(std::string_view pattern, bool first = false) const;
        inline uintptr_t findFirst(std::string_view pattern) const
        {
            auto found = find(pattern, true);
            return found.empty() ? 0 : found.front();
        }
    };
} // namespace fatigue
