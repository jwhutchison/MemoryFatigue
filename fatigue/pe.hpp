#pragma once

#include <cstdint>

namespace fatigue::pe {
    // PE (Portable Executable) format for Windows executables, DLLs, etc
    // See https://learn.microsoft.com/en-us/windows/win32/debug/pe-format

    const uint16_t DOS_MAGIC = 0x5A4D; // MZ
    const uint32_t PE_SIGNATURE = 0x00004550; // PE\0\0
    const uint16_t PE32_MAGIC = 0x10B; // PE32
    const uint16_t PE32PLUS_MAGIC = 0x20B; // PE32+

    struct DosHeader {
        uint16_t magic;
        uint8_t ignored[58];
        int32_t coff_header_offset;
    };

    struct CoffHeader {
        uint32_t signature;
        uint16_t machine;
        uint16_t number_of_sections;
        uint32_t time_date_stamp;
        uint32_t pointer_to_symbol_table;
        uint32_t number_of_symbols;
        uint16_t size_of_optional_header;
        uint16_t characteristics;
    };

    struct CoffOptionalHeader {
        uint16_t magic;
        uint8_t ignored[104];
        uint16_t number_of_rva_and_sizes;
    };

    struct SectionHeader {
        char name[8];
        uint32_t virtual_size;
        uint32_t virtual_address;
        uint8_t ignored[24];
    };
} // namespace fatigue::pe
