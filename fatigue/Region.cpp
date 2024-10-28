#include <stdexcept>
#include "mem.hpp"
#include "utils.hpp"
#include "Region.hpp"

using namespace fatigue::mem;

namespace fatigue {
    ssize_t Region::read(uintptr_t offset, void* buffer, size_t size) const
    {
        if (!isValid() || !buffer || size == 0) return -1;
        if (start + offset + size > end) throw std::out_of_range("Attempted read past end of region");

        if (method == AccessMethod::SYS) {
            return sys::read(pid, start + offset, buffer, size);
        } else if (method == AccessMethod::IO) {
            return io::read(pid, start + offset, buffer, size);
        } else {
            throw std::runtime_error("Invalid memory access method");
        }
    }

    ssize_t Region::write(uintptr_t offset, const void* buffer, size_t size) const
    {
        if (!isValid() || !buffer || size == 0) return -1;
        if (start + offset + size > end) throw std::out_of_range("Attempted write past end of region");

        if (method == AccessMethod::SYS) {
            return sys::write(pid, start + offset, buffer, size);
        } else if (method == AccessMethod::IO) {
            return io::write(pid, start + offset, buffer, size);
        } else {
            throw std::runtime_error("Invalid memory access method");
        }
    }

    std::vector<uintptr_t> Region::find(const void* pattern, size_t patternSize, const std::string& mask, bool first) const
    {
        if (!isValid() || !pattern || patternSize == 0) return {};

        // Copy the memory region into a buffer
        // TODO: Consider caching this, or maybe split the search into chunks
        std::vector<unsigned char> buffer;
        buffer.reserve(size());
        read(0, buffer.data(), size());

        // Search the buffer for the pattern
        return search::search(buffer.data(), size(), pattern, patternSize, mask, first);
    }

    std::vector<uintptr_t> Region::find(std::string_view pattern, bool first) const
    {
        if (!isValid() || pattern.empty()) return {};

        // Copy the memory region into a buffer
        // TODO: Consider caching this, or maybe split the search into chunks
        std::vector<unsigned char> buffer;
        buffer.reserve(size());
        read(0, buffer.data(), size());

        // Search the buffer for the pattern
        return search::search(buffer.data(), size(), pattern, first);
    }
} // namespace fatigue
