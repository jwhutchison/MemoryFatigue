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
    }

    std::vector<Region> ElfMap::getLoaded()
    {
        std::vector<Region> regions;
        for (auto &it : m_segments) {
            if (it.p_type == PT_LOAD) {
                regions.push_back(Region(pid, start + it.p_vaddr, start + it.p_vaddr + it.p_memsz));
            }
        }
        return regions;

    }

    Region ElfMap::getLoadedRegion()
    {
        // Get the minimum and maximum addresses of the loaded segments, create a single region
        uintptr_t min = std::numeric_limits<uintptr_t>::max();
        uintptr_t max = 0;
        for (auto &it : m_segments) {
            if (it.p_type == PT_LOAD) {
                min = std::min(min, it.p_vaddr);
                max = std::max(max, it.p_vaddr + it.p_memsz);
            }
        }
        return min < max ? Region(pid, start + min, start + max, "Loaded Regions") : Region();
    }

    std::vector<Region> ElfMap::getDynamic()
    {
        std::vector<Region> regions;
        for (auto &it : m_segments) {
            if (it.p_type == PT_DYNAMIC) {
                regions.push_back(Region(pid, start + it.p_vaddr, start + it.p_vaddr + it.p_memsz));
            }
        }
        return regions;
    }


} // namespace fatigue::pe
