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
     * with start and end addresses and optional name. Provides read and write methods
     * for reading and writing memory in the region, using the various memory access
     * methods available in the mem namespace.
     */
    class Region {
    public:
        /**
         * @brief Memory access method
         * @details The method used to access memory in the region, e.g. syscalls, /proc/pid/mem, ptrace, etc
         */
        AccessMethod method;
        /**
         * @brief Enforce bounds checking on read and write operations
         * @details If true, read and write operations will throw an exception if they would read or
         * write before the start or after the end of the region
         */
        bool enforceBounds{true};

        /** Name of the region (useful for segments) */
        std::string name;
        /** Process ID */
        pid_t pid{0};
        /** Absolute address in process memory of the start of the region */
        unsigned long long start{0};
        /** Absolute address in process memory of the end of the region */
        unsigned long long end{0};

        Region() : method(getAccessMethod()) {};
        Region(pid_t pid, uintptr_t start, uintptr_t end)
            : method(getAccessMethod()), pid(pid), start(start), end(end) {}
        Region(pid_t pid, uintptr_t start, uintptr_t end, const std::string& name)
            : method(getAccessMethod()), pid(pid), start(start), end(end), name(name) {}
        ~Region() = default;

        /** Get the size of the region in bytes */
        inline size_t size() const { return end - start; }
        /** Check if the region is valid */
        inline bool isValid() const { return pid > 0 && start >= 0 && end > 0 && end > start; }
        /** Check if an address is within the region */
        inline bool contains(uintptr_t address) const { return address >= start && address < end; }

        inline std::string toString()
        {
            return std::format("{:#x}-{:#x} {}", start, end, name.c_str());
        }

        // Read and write

        /**
         * @brief Read memory from the region
         * @param offset Offset from the start of the region
         * @param buffer Buffer to read into
         * @param size Number of bytes to read
         */
        ssize_t read(ssize_t offset, void* buffer, size_t size) const;
        /**
         * @brief Write memory to the region
         * @param offset Offset from the start of the region
         * @param buffer Buffer to write from
         * @param size Number of bytes to write
         */
        ssize_t write(ssize_t offset, const void* buffer, size_t size) const;

        /**
         * @brief Read a value from the region
         * @param offset Offset from the start of the region
         * @param value Pointer to the value of type T to read into
         */
        template <typename T>
        ssize_t read(ssize_t offset, T* value) const
        {
            return read(offset, value, sizeof(T));
        }

        /**
         * @brief Write a value to the region
         * @param offset Offset from the start of the region
         * @param value Pointer to the value of type T to write
         */
        template <typename T>
        ssize_t write(ssize_t offset, const T* value) const
        {
            return write(offset, value, sizeof(T));
        }

        // Pattern scanning

        /**
         * Find a pattern in the region using a byte literal and a mask
         * @param pattern Pointer to the byte pattern to search for
         * @param patternSize Size of the pattern in bytes
         * @param mask Mask string to indicate which bytes in the pattern are wildcards
         * @param first If true, return only the first match
         * @see fatigue::search::search()
         */
        std::vector<uintptr_t> find(const void* pattern, size_t patternSize, const std::string& mask, bool first = false) const;

        /**
         * Find the first occurrence of a pattern in the region using a byte literal and a mask
         * @param pattern Pointer to the byte pattern to search for
         * @param patternSize Size of the pattern in bytes
         * @param mask Mask string to indicate which bytes in the pattern are wildcards
         * @see fatigue::search::search()
         */
        inline uintptr_t findFirst(const void* pattern, size_t patternSize, const std::string& mask) const
        {
            auto found = find(pattern, patternSize, mask, true);
            return found.empty() ? 0 : found.front();
        }

        /**
         * Find a pattern in the region using a hex string pattern and a mask
         * @param pattern Hex string pattern to search for
         * @param first If true, return only the first match
         * @see fatigue::search::search()
         */
        std::vector<uintptr_t> find(std::string_view pattern, bool first = false) const;

        /**
         * Find the first occurrence of a pattern in the region using a hex string pattern and a mask
         * @param pattern Hex string pattern to search for
         * @see fatigue::search::search()
         */
        inline uintptr_t findFirst(std::string_view pattern) const
        {
            auto found = find(pattern, true);
            return found.empty() ? 0 : found.front();
        }
    };
} // namespace fatigue
