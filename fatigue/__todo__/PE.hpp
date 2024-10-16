#pragma once

#include "KittyMemOp.hpp"
#include "Fatigue.hpp"

namespace fatigue::pe {
    // PE (Portable Executable) format for Windows executables, DLLs, etc
    // See https://learn.microsoft.com/en-us/windows/win32/debug/pe-format

    const uint32_t DOS_MAGIC = 0x5A4D; // MZ
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

    class SectionScanner {
    public:
        SectionScanner(ProcMap* map, SectionHeader header): m_procMap(map), m_header(header) {}

        inline ProcMap* getProcMap() const { return m_procMap; }
        inline SectionHeader getHeader() const { return m_header; }
        inline std::string getName() const { return std::string(m_header.name); }

        bool isValid() const
        {
            return m_procMap && m_procMap->isValid() && m_header.virtual_size > 0;
        }

        uint32_t getSize() const { return m_header.virtual_size; }

        uintptr_t getStartAddress() const
        {
            return m_procMap->startAddress + m_header.virtual_address;
        }

        uintptr_t getEndAddress() const
        {
            return getStartAddress() + m_header.virtual_size;
        }

    protected:
        ProcMap* m_procMap;
        SectionHeader m_header{0};
    };

    class PeScannerMgr {
    public:
        PeScannerMgr() {}
        PeScannerMgr(IKittyMemOp* pMem);
        // ProcMap* getBaseProcessMap(KittyMemoryMgr& manager);

        bool isValid() const
        {
            // Valid and ready to rock if we have a valid memory operation, process map, and PE headers
            return m_pMem && m_pMem->processID() > 0
                && m_procMap && m_procMap->isValid()
                && m_dosHeader.magic == DOS_MAGIC
                && m_coffHeader.signature == PE_SIGNATURE
                && (m_coffOptionalHeader.magic == PE32PLUS_MAGIC || m_coffOptionalHeader.magic == PE32_MAGIC)
                && !m_sectionHeaders.empty();
        };

        inline pid_t getPid() { return m_pMem ? m_pMem->processID() : 0; }

        inline ProcMap* getProcMap() { return m_procMap; }
        inline uintptr_t getStartAddress() const { return m_procMap ? m_procMap->startAddress : 0; }
        inline uintptr_t getEndAddress() const { return m_procMap ? m_procMap->endAddress : 0; }

        // Kind of compatibility with ElfScanner
        inline auto base() { return getStartAddress(); }
        inline auto end() { return getEndAddress(); }

        inline auto getDosHeader() { return m_dosHeader; }
        inline auto getCoffHeader() { return m_coffHeader; }
        inline auto getCoffOptionalHeader() { return m_coffOptionalHeader; }
        inline auto getSectionHeaders() { return m_sectionHeaders; }

        SectionScanner* getSection(const std::string& name = ".text");
        void dump();

    protected:
        IKittyMemOp* m_pMem{nullptr};
        ProcMap* m_procMap{nullptr};

        DosHeader m_dosHeader{0};
        CoffHeader m_coffHeader{0};
        CoffOptionalHeader m_coffOptionalHeader{0};
        std::vector<SectionHeader> m_sectionHeaders{};
    };
} // namespace fatigue::pe
