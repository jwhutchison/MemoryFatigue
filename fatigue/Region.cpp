#include "mem.hpp"
#include "Region.hpp"

using namespace fatigue::mem;

namespace fatigue {
    ssize_t Region::read(uintptr_t offset, void* buffer, size_t size) const
    {
        if (!isValid() || !buffer || size == 0) return -1;
        if (m_start + offset + size > m_end) throw std::out_of_range("Attempted read past end of region");

        switch (method) {
        case AccessMethod::SYS:
            return sys::read(m_pid, m_start + offset, buffer, size);
        case AccessMethod::IO:
            return io::read(m_pid, m_start + offset, buffer, size);
        default:
            return -1;
        }
    }

    ssize_t Region::write(uintptr_t offset, const void* buffer, size_t size) const
    {
        if (!isValid() || !buffer || size == 0) return -1;
        if (m_start + offset + size > m_end) throw std::out_of_range("Attempted write past end of region");

        switch (method) {
        case AccessMethod::SYS:
            return sys::write(m_pid, m_start + offset, buffer, size);
        case AccessMethod::IO:
            return io::write(m_pid, m_start + offset, buffer, size);
        default:
            return -1;
        }
    }
} // namespace fatigue
