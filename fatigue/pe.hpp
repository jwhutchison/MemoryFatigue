#pragma once

#include <cstdint>
#include "log.hpp"
#include "proc.hpp"
#include "Region.hpp"

/**
 * @brief Portable Executable (PE) format for Windows executables, DLLs, etc
 * i.e. running via Wine, Proton, etc
 * @see https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
 */
namespace fatigue::pe {
    const uint16_t DOS_MAGIC = 0x5A4D; // MZ
    const uint32_t PE_SIGNATURE = 0x00004550; // PE\0\0
    const uint16_t PE32_MAGIC = 0x10B; // PE32
    const uint16_t PE32PLUS_MAGIC = 0x20B; // PE32+

    struct DosHeader {
        uint16_t magic;
        uint8_t ignored[58];
        int32_t coffHeaderOffset;
    };

    struct CoffHeader {
        uint32_t signature;
        uint16_t machine;
        uint16_t sectionCount;
        uint32_t timestamp;
        uint32_t symbolTablePointer;
        uint32_t symbolCount;
        uint16_t optionalHeaderSize;
        uint16_t characteristics;
    };

    struct CoffOptionalHeader {
        uint16_t magic;
        uint8_t ignored[104];
        uint16_t rvaSizes;
    };

    struct SectionHeader {
        char name[8];
        uint32_t virtualSize;
        uint32_t virtualAddress;
        uint8_t ignored[24]; // Allows a block read to fetch all section headers
    };

    bool isValidPE(pid_t pid, uintptr_t address);
    bool isValidPE(proc::Map& map);

    class PeMap : public proc::Map {
    protected:
        DosHeader m_dos{0};
        CoffHeader m_coff{0};
        CoffOptionalHeader m_optional{0};
        std::vector<SectionHeader> m_sections{};
    public:
        PeMap() = default;
        PeMap(proc::Map& map, bool autoInit = true) : proc::Map(map) {
            // Deferring init allows you to change memory access method before any reads happen
            // However, sys should work fine for reading PE headers
            if (autoInit) init();
        }
        ~PeMap() = default;

        DosHeader dos() const { return m_dos; }
        CoffHeader coff() const { return m_coff; }
        CoffOptionalHeader optional() const { return m_optional; }
        std::vector<SectionHeader> sections() const { return m_sections; }

        void init();

        inline bool isValidDos() const { return m_dos.magic == DOS_MAGIC; }
        inline bool isValidCoff() const { return m_coff.signature == PE_SIGNATURE; }
        inline bool isValidOptional() const { return m_optional.magic == PE32_MAGIC || m_optional.magic == PE32PLUS_MAGIC; }

        inline bool isValid() const
        {
            return proc::Map::isValid() && isValidDos() && isValidCoff() && isValidOptional();
        }

        std::vector<Region> getSections();
        Region getSection(const std::string_view &name = ".text");
    };

} // namespace fatigue::pe
