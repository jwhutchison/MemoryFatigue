#include "elf.hpp"

using namespace fatigue::mem::sys;

namespace fatigue::elf {
    bool isValidElf(pid_t pid, uintptr_t address) {
        ElfHeader header;
        sys::read(pid, address, &header, sizeof(header));
        return header.magic == ELF_MAGIC;
    }

    bool isValidElf(proc::Map& map) {
        if (!map.isValid() || map.isAnonymous()) return false;
        return isValidElf(map.pid, map.start);
    }

    void ElfMap::init() {
        if (!proc::Map::isValid()) return;

        // Read ELF header, and check if it's a ELF executable
        read(0, &m_header, sizeof(m_header));
        if (m_header.magic != ELF_MAGIC) {
            logError(std::format("Invalid ELF header for {} {}", pid, name));
            return;
        }

        // If not ELF64, return
        if (m_header.classType != 2) {
            logError(std::format("Invalid ELF (not 64-bit) for {} {}", pid, name));
            return;
        }

        // Get program headers in a single read
        m_programs.resize(m_header.phnum);
        read(
            m_header.phoff,
            m_programs.data(),
            m_header.phentsize * m_header.phnum
        );
        if (m_programs.empty() || m_programs.size() != m_header.phnum) {
            logError(std::format("Invalid program headers for {} {}", pid, name));
            return;
        }

        // Get section headers in a single read
        m_sections.resize(m_header.shnum);
        read(
            m_header.shoff,
            m_sections.data(),
            m_header.shentsize * m_header.shnum
        );
        if (m_sections.empty() || m_sections.size() != m_header.shnum) {
            logError(std::format("Invalid section headers for {} {}", pid, name));
            return;
        }

        // Read the shared strings section
        if (m_header.shstrndx < m_sections.size()) {
            ElfSectionHeader &section = m_sections.at(m_header.shstrndx);
            m_sharedStrings.resize(section.size);
            read(section.offset, m_sharedStrings.data(), section.size);
        }
    }

    std::vector<Region> ElfMap::getSections()
    {
        std::vector<Region> sections;
        for (auto &it : m_sections) {
            std::string name(reinterpret_cast<char*>(m_sharedStrings.data() + it.name));
            name = string::trim(name);
            sections.push_back(Region(pid, start + it.addr, start + it.addr + it.size, name));
        }
        return sections;
    }

    Region ElfMap::getSection(const std::string_view &name)
    {
        for (auto &it : m_sections) {
            std::string secName(reinterpret_cast<char*>(m_sharedStrings.data() + it.name));
            secName = string::trim(secName);
            // if secion name starts with a ., match with or without the dot
            if (name == secName || (secName.starts_with(".") && name == secName.substr(1)))
                return Region(pid, start + it.addr, start + it.addr + it.size, secName);
        }
        // Return an invalid region if not found
        return Region();
    }
} // namespace fatigue::pe
