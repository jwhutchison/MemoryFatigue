#include "elf.hpp"

using namespace fatigue::mem::sys;

namespace fatigue::elf {
    bool isValidElf(pid_t pid, uintptr_t address) {
        std::byte header[4];
        sys::read(pid, address, &header, sizeof(header));

        std::string magic = std::string((char*)&header[0], 4);
        return magic == ELFMAG;
    }

    bool isValidElf(proc::Map& map) {
        if (!map.isValid() || map.isAnonymous()) return false;
        return isValidElf(map.pid, map.start);
    }

    void ElfMap::init() {
        if (!proc::Map::isValid()) return;

        enforceBounds = false;

        logInfo(std::format("Initializing ELF map for {} {}", pid, name));

        // Read ELF header, and check if it's a ELF executable
        read(0, &m_header, sizeof(m_header));

        if (!isValidElf() || m_header.e_ident[EI_CLASS] != ELF_CLASS) {
            logError(std::format("Invalid ELF header for {} {}", pid, name));
            return;
        }

        // std::string ident = hex::toPrettyHex(&m_header.e_ident, 16);

        // logInfo(std::format("ELF Header fields:\n"
        //                     "  Ident: {}\n"
        //                     "  Type: {}\n"
        //                     "  Machine: {}\n"
        //                     "  Version: {}\n"
        //                     "  Entry: {:#x}\n"
        //                     "  Program Header Offset: {} {:#x}\n"
        //                     "  Section Header Offset: {} {:#x}\n"
        //                     "  Flags: {:#x}\n"
        //                     "  ELF Header Size: {}\n"
        //                     "  Program Header Entry Size: {} {:#x}\n"
        //                     "  Program Header Entry Count: {}\n"
        //                     "  Section Header Entry Size: {} {:#x}\n"
        //                     "  Section Header Entry Count: {}\n"
        //                     "  Section Header String Table Index: {}\n"
        //                     "{}",
        //                     ident,
        //                     m_header.e_type, m_header.e_machine, m_header.e_version,
        //                     m_header.e_entry,
        //                     m_header.e_phoff, m_header.e_phoff,
        //                     m_header.e_shoff, m_header.e_shoff,
        //                     m_header.e_flags,
        //                     m_header.e_ehsize,
        //                     m_header.e_phentsize, m_header.e_phentsize,
        //                     m_header.e_phnum,
        //                     m_header.e_shentsize, m_header.e_shentsize,
        //                     m_header.e_shnum,
        //                     m_header.e_shstrndx,
        //                     ""
        // ));

        // logInfo(std::format("Reading {} program headers of size {} starting at {:#x} {:#x}",
        //                     m_header.e_phnum, m_header.e_phentsize,
        //                     m_header.e_phoff, start + m_header.e_phoff));
        // Get program headers in a single read
        m_segments.resize(m_header.e_phnum);
        read(
            m_header.e_phoff,
            m_segments.data(),
            m_header.e_phentsize * m_header.e_phnum
        );
        if (m_segments.empty() || m_segments.size() != m_header.e_phnum) {
            logError(std::format("Invalid segment headers for {} {}", pid, name));
            return;
        }

        // logInfo("Segment Headers:");
        // for (auto &it : m_segments) {
        //     std::cout << std::format(
        //         "Segment Header: {:#x} {:#x} {:#x}",
        //         it.p_type, it.p_vaddr, it.p_memsz
        //     ) << std::endl;

        //     if (it.p_type == PT_LOAD) {
        //         std::cout << "  Loadable Segment" << std::endl;
        //     }

        //     if (it.p_type == PT_DYNAMIC) {
        //         std::cout << "  Dynamic Segment" << std::endl;
        //     }
        // }
        // std::cout << "  ...Ok" << std::endl;
    }

    std::vector<Region> ElfMap::getSections()
    {
        std::vector<Region> sections;
        for (auto &it : m_segments) {
            // std::string name(reinterpret_cast<char*>(m_sharedStrings.data() + it.name));
            // name = string::trim(name);
            if (it.p_type != PT_LOAD) continue;
            sections.push_back(Region(pid, start + it.p_vaddr, start + it.p_vaddr + it.p_memsz));
        }
        return sections;
    }

    Region ElfMap::getSection(const std::string_view &name)
    {
        for (auto &it : m_segments) {
            // std::string secName(reinterpret_cast<char*>(m_sharedStrings.data() + it.name));
            // secName = string::trim(secName);
            // // if secion name starts with a ., match with or without the dot
            // if (name == secName || (secName.starts_with(".") && name == secName.substr(1)))
            //     return Region(pid, start + it.addr, start + it.addr + it.size, secName);
        }
        // Return an invalid region if not found
        return Region();
    }
} // namespace fatigue::pe
