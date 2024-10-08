#include "Fatigue.hpp"

namespace fatigue::pe {
    // PE (Portable Executable) format for Windows executables, DLLs, etc
    // See https://learn.microsoft.com/en-us/windows/win32/debug/pe-format

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

    class Section {
    public:
        Section(): m_procMap(nullptr), m_header{0} {}
        Section(ProcMap* procMap, SectionHeader header): m_procMap(procMap), m_header(header) {}

        inline ProcMap* getProcMap() { return m_procMap; }
        inline SectionHeader getHeader() { return m_header; }

        std::string getName()
        {
            return std::string(m_header.name);
        }

        bool isValid()
        {
            return m_procMap->isValid() && m_header.virtual_size > 0;
        }

        uintptr_t getStartAddress()
        {
            return m_procMap->startAddress + m_header.virtual_address;
        }

        uintptr_t getEndAddress()
        {
            return getStartAddress() + m_header.virtual_size;
        }
    protected:
        ProcMap* m_procMap;
        SectionHeader m_header;
    };

    class ProcMap : public fatigue::ProcMap {
    public:
        DosHeader dos_header{0};
        CoffHeader coff_header{0};
        CoffOptionalHeader coff_optional_header{0};
        std::vector<SectionHeader> section_headers{};

        ProcMap(): fatigue::ProcMap() {}
        ProcMap(fatigue::ProcMap& map): fatigue::ProcMap(map)
        {
            pid = map.pid;
            startAddress = map.startAddress;
            endAddress = map.endAddress;
            length = map.length;
            protection = map.protection;
            readable = map.readable;
            writeable = map.writeable;
            executable = map.executable;
            is_private = map.is_private;
            is_shared = map.is_shared;
            is_ro = map.is_ro;
            is_rw = map.is_rw;
            is_rx = map.is_rx;
            offset = map.offset;
            dev = std::string(map.dev);
            inode = map.inode;
            pathname = std::string(map.pathname);
        }

        Section* getSection(const std::string& name = ".text");
        void dump();
    };

    ProcMap* getBaseProcessMap(KittyMemoryMgr& manager);
} // namespace fatigue::pe
