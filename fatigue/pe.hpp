#pragma once

#include <cstdint>
#include "proc.hpp"

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
        uint8_t ignored[24];
    };

    bool isValidPE(pid_t pid, uintptr_t address);
    bool isValidPE(proc::Map& map);
} // namespace fatigue::pe
