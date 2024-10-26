#include "mem.hpp"
#include "Region.hpp"
#include <stdexcept>

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
} // namespace fatigue
