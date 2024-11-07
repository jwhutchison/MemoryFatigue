#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "Region.hpp"

namespace fatigue {
    class Patch {
    protected:
        Region m_region{};
        std::string m_pattern;
        bool m_first{true};
        int m_offset{0};
        std::vector<uint8_t> m_original{};
        std::vector<uint8_t> m_patch{};
        std::vector<uintptr_t> m_matches{};
        bool m_found{false};
        bool m_applied{false};
    public:
        Patch() = default;
        Patch(const Region& region, const std::string& pattern, const std::vector<uint8_t>& patch, int offset, bool first = true)
            : m_region(region), m_pattern(pattern), m_patch(patch), m_offset(offset), m_first(first) {}
        ~Patch() = default;




        static void apply(uintptr_t address, const void* patch, size_t size);
        static void restore(uintptr_t address, const void* patch, size_t size);
    };
}