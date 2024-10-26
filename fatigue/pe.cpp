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
        logInfo(std::format("Initializing PE map for {} {}", pid, name));
        if (!isValid()) return;

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
        std::vector<SectionHeader> sections(m_coff.sectionCount);
        read(
            m_dos.coffHeaderOffset + sizeof(m_coff) + m_coff.optionalHeaderSize,
            sections.data(),
            sizeof(SectionHeader) * m_coff.sectionCount
        );
    }

    bool PeMap::isValid() const {
        logInfo(std::format("Checking PE map for {} {}", pid, name));
        if (!proc::Map::isValid()) return false;
        logInfo(std::format("Checking PE map for {} {} 2", pid, name));
        if (m_dos.magic != DOS_MAGIC) return false;
        logInfo(std::format("Checking PE map for {} {} 3", pid, name));
        if (m_coff.signature != PE_SIGNATURE) return false;
        logInfo(std::format("Checking PE map for {} {} 4", pid, name));
        if (m_optional.magic != PE32_MAGIC && m_optional.magic != PE32PLUS_MAGIC) return false;
        logInfo(std::format("Checking PE map for {} {} 5", pid, name));
        return true;
    }

    Region PeMap::getSection(const std::string_view &name)
    {
        for (auto &it : m_sections) {
            // if secion name starts with a ., match with or without the dot
            if (name == it.name || (name[0] != '.' && name == std::string(it.name + 1))) {
                return Region(pid, it.virtualAddress, it.virtualAddress + it.virtualSize, it.name);
            }
        }
        return Region();
    }
} // namespace fatigue::pe
