#include "pe.hpp"

using namespace fatigue::mem::sys;

namespace fatigue::pe {
    bool isValidPE(pid_t pid, uintptr_t address) {
        DosHeader dos;
        sys::read(pid, address, &dos, sizeof(dos));
        return dos.magic == DOS_MAGIC;
    }

    bool isValidPE(proc::Map& map) {
        if (!map.isValid() || map.isAnonymous()) return false;
        return isValidPE(map.pid, map.start);
    }

    void PeMap::init() {
        if (!proc::Map::isValid()) return;

        // Read DOS header, and check if it's a DOS executable
        read(0, &m_dos, sizeof(m_dos));
        if (m_dos.magic != DOS_MAGIC) {
            logError(std::format("Invalid DOS header for {} {}", pid, name));
            return;
        }

        // Read COFF header, and check PE signature
        read(m_dos.coffHeaderOffset, &m_coff, sizeof(m_coff));
        if (m_coff.signature != PE_SIGNATURE) {
            logError(std::format("Invalid COFF header for {} {}", pid, name));
            return;
        }

        // Read COFF optional header, and check PE32+ magic number
        std::size_t optionalSize = std::min(
            // Do not read more than the size of the optional header, but also no more than the size of the struct
            static_cast<std::size_t>(m_coff.optionalHeaderSize),
            sizeof(m_optional)
        );
        read(m_dos.coffHeaderOffset + sizeof(m_coff), &m_optional, sizeof(m_optional));
        if (m_optional.magic != PE32_MAGIC && m_optional.magic != PE32PLUS_MAGIC) {
            logError(std::format("Invalid COFF optional header for {} {}", pid, name));
            return;
        }

        // Get section headers in a single read
        m_sections.resize(m_coff.sectionCount);
        read(
            m_dos.coffHeaderOffset + sizeof(m_coff) + m_coff.optionalHeaderSize,
            m_sections.data(),
            sizeof(SectionHeader) * m_coff.sectionCount
        );
        if (m_sections.empty() || m_sections.size() != m_coff.sectionCount) {
            logError(std::format("Invalid section headers for {} {}", pid, name));
            return;
        }
    }

    std::vector<Region> PeMap::getSections()
    {
        std::vector<Region> sections;
        for (auto &it : m_sections) {
            std::string name(it.name, sizeof(it.name));
            name = string::trim(name);
            sections.push_back(Region(pid, start + it.virtualAddress, start + it.virtualAddress + it.virtualSize, name));
        }
        return sections;
    }

    Region PeMap::getSection(const std::string_view &name)
    {
        for (auto &it : m_sections) {
            std::string secName = string::trim(it.name);
            // if secion name starts with a ., match with or without the dot
            if (name == secName || (secName.starts_with(".") && name == secName.substr(1)))
                return Region(pid, start + it.virtualAddress, start + it.virtualAddress + it.virtualSize, secName);
        }
        // Return an invalid region if not found
        return Region();
    }
} // namespace fatigue::pe
