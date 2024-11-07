#include <cstring>
#include <errno.h>
#include <stdexcept>
#include "log.hpp"
#include "mem.hpp"
#include "utils.hpp"
#include "Region.hpp"

using namespace fatigue::mem;

namespace fatigue {
    ssize_t Region::read(ssize_t offset, void* buffer, size_t size) const
    {
        if (!isValid() || !buffer || size == 0) return -1;
        if (enforceBounds) {
            if (offset < 0) throw std::out_of_range("Attempted read before start of region");
            if (start + offset + size > end) throw std::out_of_range("Attempted read past end of region");
        }

        ssize_t bytesRead = 0;
        errno = 0;

        if (method == AccessMethod::SYS) {
            bytesRead = sys::read(pid, start + offset, buffer, size);
        } else if (method == AccessMethod::IO) {
            bytesRead = io::read(pid, start + offset, buffer, size);
        } else if (method == AccessMethod::PTRACE) {
            bytesRead = trace::read(pid, start + offset, buffer, size);
        } else {
            throw std::runtime_error("Invalid memory access method");
        }

        if (bytesRead < 0) {
            logError(std::format("Failed to read {} bytes from {:#x}-{:#x}: {}", size, start + offset, start + offset + size, strerror(errno)));
            throw std::runtime_error(std::format("Read error: {}", strerror(errno)));
        }

        return bytesRead;
    }

    ssize_t Region::write(ssize_t offset, const void* buffer, size_t size) const
    {
        if (!isValid() || !buffer || size == 0) return -1;
        if (enforceBounds) {
            if (offset < 0) throw std::out_of_range("Attempted write before start of region");
            if (start + offset + size > end) throw std::out_of_range("Attempted write past end of region");
        }

        ssize_t bytesWritten = 0;
        errno = 0;

        if (method == AccessMethod::SYS) {
            bytesWritten = sys::write(pid, start + offset, buffer, size);
        } else if (method == AccessMethod::IO) {
            bytesWritten = io::write(pid, start + offset, buffer, size);
        } else if (method == AccessMethod::PTRACE) {
            bytesWritten = trace::write(pid, start + offset, buffer, size);
        } else {
            throw std::runtime_error("Invalid memory access method");
        }

        if (bytesWritten < 0) {
            logError(std::format("Failed to write {} bytes to {:#x}-{:#x}: {}", size, start + offset, start + offset + size, strerror(errno)));
            throw std::runtime_error(std::format("Write error: {}", strerror(errno)));
        }

        return bytesWritten;
    }

    std::vector<uintptr_t> Region::find(const void* pattern, size_t patternSize, const std::string& mask, bool first) const
    {
        if (!isValid() || !pattern || patternSize == 0) return {};

        // Copy the memory region into a buffer
        // TODO: Save this in the object for multiple searches, allow refresh
        std::vector<uint8_t> buffer;
        buffer.reserve(size());
        read(0, buffer.data(), size());

        // Search the buffer for the pattern
        return search::search(buffer.data(), size(), pattern, patternSize, mask, first);
    }

    std::vector<uintptr_t> Region::find(std::string_view pattern, bool first) const
    {
        if (!isValid() || pattern.empty()) return {};

        // Copy the memory region into a buffer
        std::vector<uint8_t> buffer;
        buffer.reserve(size());
        read(0, buffer.data(), size());

        // Search the buffer for the pattern
        return search::search(buffer.data(), size(), pattern, first);
    }
} // namespace fatigue
