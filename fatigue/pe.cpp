#include "log.hpp"
#include "pe.hpp"
#include "Region.hpp"

using namespace fatigue::mem::sys;

namespace fatigue::pe {
    bool isValidPE(pid_t pid, uintptr_t address) {
        return false;
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

        // Read section headers
        logInfo(std::format("Reading {} section headers", m_coff.sectionCount));
        m_sections.resize(m_coff.sectionCount);
        read(
            m_dos.coffHeaderOffset + sizeof(m_coff) + m_coff.optionalHeaderSize,
            m_sections.data(),
            sizeof(SectionHeader) * m_coff.sectionCount
        );
        logInfo(hex::dump(m_sections.data(), sizeof(SectionHeader) * m_coff.sectionCount, 16));
    }

    std::vector<Region> PeMap::getSections()
    {
        std::vector<Region> sections;
        for (auto &it : m_sections) {
            std::string name(it.name, sizeof(it.name));
            name = string::trim(name);
            sections.push_back(Region(pid, it.virtualAddress, it.virtualAddress + it.virtualSize, name));
        }
        return sections;
    }

    Region PeMap::getSection(const std::string_view &name)
    {
        for (auto &it : getSections()) {
            // if secion name starts with a ., match with or without the dot
            if (name == it.name || (name[0] != '.' && name == it.name.substr(1)))
                return it;
        }
        // Return an invalid region if not found
        return Region();
    }
} // namespace fatigue::pe
