#pragma once

#include <cstdint>
#include <elf.h>
#include "log.hpp"
#include "proc.hpp"
#include "Region.hpp"

#ifdef __LP64__
#define ELF_CLASS 2
#define ELF_BITS 64
#define Elf_(x) Elf64_##x
#define ELF_(x) ELF64_##x
#else
#define ELF_CLASS 1
#define ELF_BITS 32
#define Elf_(x) Elf32_##x
#define ELF_(x) ELF32_##x
#endif

#define ElfHeader Elf_(Ehdr)
#define ElfSegmentHeader Elf_(Phdr)

/**
 * @brief Executable and Linkable Format (ELF) for Linux executables, shared libraries, etc
 * Note: Assumes 64-bit ELF format by default, use 32-bit structs for 32-bit ELF
 * @see https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
 */
namespace fatigue::elf {
    bool isValidElf(pid_t pid, uintptr_t address);
    bool isValidElf(proc::Map& map);

    class ElfMap : public proc::Map {
    protected:
        ElfHeader m_header{0};
        std::vector<ElfSegmentHeader> m_segments{};
        std::vector<uint8_t> m_sharedStrings{};
    public:
        ElfMap() = default;
        ElfMap(proc::Map& map, bool autoInit = true) : proc::Map(map) {
            // Deferring init allows you to change memory access method before any reads happen
            // However, sys should work fine for reading headers
            if (autoInit) init();
        }
        ~ElfMap() = default;

        ElfHeader header() const { return m_header; }
        std::vector<ElfSegmentHeader> segments() const { return m_segments; }
        std::vector<uint8_t> sharedStrings() const { return m_sharedStrings; }

        void init();

        inline bool isValidElf() const {
            std::string magic((char*)&m_header.e_ident[0], 4);
            return magic == ELFMAG;
        }

        inline bool isValid() const
        {
            return proc::Map::isValid() && isValidElf();
        }

        /**
         * @brief Get all loaded segments from the ELF as regions
         */
        std::vector<Region> getLoaded();

        /**
         * @brief Get a single region for the entire loaded ELF
         * @warning This is the laziest possible implementation to get loaded segments, but it should work for simple pattern scans
         */
        Region getLoadedRegion();

        /**
         * @brief Get all dynamic segments from the ELF as regions
         */
        std::vector<Region> getDynamic();
    };
} // namespace fatigue::elf
