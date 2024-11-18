#pragma once

#include <cstdint>
#include <format>
#include <functional>
#include <iostream>
#include <map>
#include <vector>
#include "log.hpp"
#include "Region.hpp"
#include "utils.hpp"

namespace fatigue {
    class Patch {
    protected:
        /** Character width of the first colomn in dump outputs */
        static const size_t labelWidth = sizeof(uintptr_t) + 2; // "0x123456: ", "Pattern:  "
        static const size_t defaultShowMatches = 5;

        Region m_region{};

        uintptr_t m_address{0};
        std::string m_pattern;

        int m_offset{0};
        std::function<int(Patch const&)> m_offset_fn{nullptr};

        std::vector<uint8_t> m_matched{};
        std::vector<uint8_t> m_original{};
        std::vector<uint8_t> m_patch{};

        bool m_found{false};
        bool m_applied{false};

        /** Not used internally, but can be used to debug multiple matches */
        std::vector<uintptr_t> m_matches{};
        std::string dumpPatternAt(long address, Color highlight = Color::Reset) const;

    public:
        Patch() = default;

        /** @brief Initialize a patch with a region, address, and patch data */
        Patch(const Region& region, uintptr_t address, const std::vector<uint8_t>& patch)
            : m_region(region), m_address(address), m_patch(patch)
        {
            m_found = true;
            init();
        }
        /** @brief Initialize a patch with a region, address, and patch hex string */
        Patch(const Region& region, uintptr_t address, const std::string& patch)
            : Patch(region, address, hex::parse(patch)) {}

        /** @brief Initialize a patch with a region, pattern, offset, and patch data */
        Patch(const Region& region, const std::string& pattern, int offset, const std::vector<uint8_t>& patch)
            : m_region(region), m_pattern(pattern), m_offset(offset), m_patch(patch)
        {
            init();
        }
        /** @brief Initialize a patch with a region, pattern, offset, and patch hex string */
        Patch(const Region& region, const std::string& pattern, int offset, const std::string& patch)
            : Patch(region, pattern, offset, hex::parse(patch)) {}

        /** @brief Initialize a patch with a region, pattern, offset function, and patch data */
        Patch(const Region& region, const std::string& pattern, std::function<int(Patch const&)> offset_fn, const std::vector<uint8_t>& patch)
            : m_region(region), m_pattern(pattern), m_offset_fn(offset_fn), m_patch(patch)
        {
            m_offset = offset();
            init();
        }
        /** @brief Initialize a patch with a region, pattern, offset function, and patch hex string */
        Patch(const Region& region, const std::string& pattern, std::function<int(Patch const&)> offset_fn, const std::string& patch)
            : Patch(region, pattern, offset_fn, hex::parse(patch)) {}

        ~Patch() = default;

        // Accessors

        inline Region region() const { return m_region; }
        inline uintptr_t address() const { return m_address; }
        inline std::string pattern() const { return m_pattern; }
        inline int offset() const { return m_offset_fn ? m_offset_fn(*this) : m_offset; }
        inline std::vector<uint8_t> patch() const { return m_patch; }
        inline std::vector<uint8_t> original() const { return m_original; }
        inline bool found() const { return m_found; }
        inline bool applied() const { return m_applied; }
        inline std::vector<uintptr_t> matches() const { return m_matches; }

        bool isValid() const { return m_region.isValid() && m_found; }
        size_t patternSize() const { return hex::split(m_pattern).size(); }
        uintptr_t patchAddress() const { return m_address + offset(); }

        /**
         * @brief Initialize the patch by finding the address and backing up the original data
         */
        void init();

        /**
         * @brief Find the address of the pattern in the region
         * @note This is called automatically by init()
         * This function will output a warning to stderr if the pattern is not found (invalid)         * @see dumpPattern(), dumpPatch()

         * or if multiple matches are found (first match is used, but this might be a mistake)
         */
        void find();

        /**
         * @brief Make a copy of the matched data at the address and the original data at the patch address
         * @note This is called automatically by init()
         */
        void backup();

        /**
         * @brief Apply the patch to the region
         * @return true if the patch was successfully applied
         */
        bool apply();

        /**
         * @brief Restore the original data to the region
         * @return true if the patch was successfully restored
         */
        bool restore();

        /**
         * @brief Toggle the patch (apply if not applied, restore if applied)
         * @return true if the patch was successfully toggled
         */
        bool toggle();

        /**
         * @brief Short string representation of the patch
         */
        std::string toString() const;

        /**
         * @brief Create a pretty representation of the entire patch
         * Useful for debugging and logging
         * This will include regiom, pattern, and patch data
         * @see dumpPattern(), dumpPatch()
         */
        std::string dump() const;

        /**
         * @brief Create a pretty representation of the pattern and matched data
         * @param showMatches If greater than 0, override the default number of matches to show
         */
        std::string dumpPattern(size_t showMatches = defaultShowMatches) const;

        std::string dumpPatch() const;
    };
}