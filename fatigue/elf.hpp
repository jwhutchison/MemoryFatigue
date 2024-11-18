#pragma once

#include <cstdint>
#include "log.hpp"
#include "proc.hpp"
#include "Region.hpp"

/**
 * @brief Executable and Linkable Format (ELF) for Linux executables, shared libraries, etc
 * Note: Assumes 64-bit ELF format by default, use 32-bit structs for 32-bit ELF
 * @see https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
 */
namespace fatigue {
    namespace elf {
        const uint32_t ELF_MAGIC = 0x464C457F; // 0x7F 'ELF'

        struct ElfHeader {
            uint32_t magic;
            uint8_t classType;
            uint8_t dataEncoding;
            uint8_t version;
            uint8_t osAbi;
            uint8_t abiVersion;
            uint8_t pad[7];
            uint16_t type;
            uint16_t machine;
            uint32_t version2;
            uint64_t entry;
            uint64_t phoff;
            uint64_t shoff;
            uint32_t flags;
            uint16_t ehsize;
            uint16_t phentsize;
            uint16_t phnum;
            uint16_t shentsize;
            uint16_t shnum;
            uint16_t shstrndx;
        };

        struct ElfProgramHeader {
            uint32_t type;
            uint32_t flags;
            uint64_t offset;
            uint64_t vaddr;
            uint64_t paddr;
            uint64_t filesz;
            uint64_t memsz;
            uint64_t align;
        };

        struct ElfSectionHeader {
            uint32_t name;
            uint32_t type;
            uint64_t flags;
            uint64_t addr;
            uint64_t offset;
            uint64_t size;
            uint32_t link;
            uint32_t info;
            uint64_t addralign;
            uint64_t entsize;
        };

        bool isValidElf(pid_t pid, uintptr_t address);
        bool isValidElf(proc::Map& map);

        class ElfMap : public proc::Map {
        protected:
            ElfHeader m_header{0};
            std::vector<ElfProgramHeader> m_programs{};
            std::vector<ElfSectionHeader> m_sections{};
            std::vector<uint8_t> m_sharedStrings{};
        public:
            ElfMap() = default;
            ElfMap(proc::Map& map, bool autoInit = true) : proc::Map(map) {
                // Deferring init allows you to change memory access method before any reads happen
                // However, sys should work fine for reading PE headers
                if (autoInit) init();
            }
            ~ElfMap() = default;

            ElfHeader header() const { return m_header; }
            std::vector<ElfProgramHeader> programs() const { return m_programs; }
            std::vector<ElfSectionHeader> sections() const { return m_sections; }
            std::vector<uint8_t> sharedStrings() const { return m_sharedStrings; }

            void init();

            inline bool isValidElf() const { return m_header.magic == ELF_MAGIC; }

            inline bool isValid() const
            {
                return proc::Map::isValid() && isValidElf();
            }

            std::vector<Region> getSections();
            Region getSection(const std::string_view &name = ".text");
        };
    } // namespace fatigue::elf

    namespace elf32 {
        struct ElfHeader {
            uint32_t magic;
            uint8_t classType;
            uint8_t dataEncoding;
            uint8_t version;
            uint8_t osAbi;
            uint8_t abiVersion;
            uint8_t pad[7];
            uint16_t type;
            uint16_t machine;
            uint32_t version2;
            uint32_t entry;
            uint32_t phoff;
            uint32_t shoff;
            uint32_t flags;
            uint16_t ehsize;
            uint16_t phentsize;
            uint16_t phnum;
            uint16_t shentsize;
            uint16_t shnum;
            uint16_t shstrndx;
        };

        struct ElfProgramHeader {
            uint32_t type;
            uint32_t offset;
            uint32_t vaddr;
            uint32_t paddr;
            uint32_t filesz;
            uint32_t memsz;
            uint32_t flags;
            uint32_t align;
        };

        struct ElfSectionHeader {
            uint32_t name;
            uint32_t type;
            uint32_t flags;
            uint32_t addr;
            uint32_t offset;
            uint32_t size;
            uint32_t link;
            uint32_t info;
            uint32_t addralign;
            uint32_t entsize;
        };

        // TODO
        // class ElfMap32 : public ElfMap {
        // protected:
        //     std::vector<ElfProgramHeader32> m_programs{};
        //     std::vector<ElfSectionHeader32> m_sections{};
        // public:
        //     ElfMap32() = default;
        //     ElfMap32(proc::Map& map, bool autoInit = true) : ElfMap(map, autoInit) {}
        //     ~ElfMap32() = default;

        //     ElfHeaderTables32 tables() const { return m_tables; }
        //     std::vector<ElfProgramHeader32> programs() const { return m_programs; }
        //     std::vector<ElfSectionHeader32> sections() const { return m_sections; }

        //     void init();
        // };
    } // namespace fatigue::elf32
} // namespace fatigue
