#include "Fatigue.hpp"

namespace Fatigue::Win32 {
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

    class ProcSection {
    public:
        ProcMap* proc_map{};
        SectionHeader header{0};

        std::string getName()
        {
            return std::string(header.name);
        }

        bool isValid()
        {
            return header.virtual_size > 0;
        }

        uintptr_t getStartAddress()
        {
            return proc_map->startAddress + header.virtual_address;
        }

        uintptr_t getEndAddress()
        {
            return getStartAddress() + header.virtual_size;
        }
    };

    class ProcMap : public Fatigue::ProcMap {
    public:
        DosHeader dos_header{0};
        CoffHeader coff_header{0};
        CoffOptionalHeader coff_optional_header{0};
        std::vector<SectionHeader> section_headers{};

        ProcMap() : Fatigue::ProcMap() {}
        ProcMap(Fatigue::ProcMap &map) : Fatigue::ProcMap(map) {
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

        ProcSection* getSection(const std::string &name = ".text");
        void dump();
    };

    ProcMap* getBaseProcessMap(KittyMemoryMgr &manager);
}
