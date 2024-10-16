#include "KittyMemoryMgr.hpp"
#include "KittyMemOp.hpp"
#include "Fatigue.hpp"
#include "PE.hpp"
#include "utils.hpp"

namespace fatigue::pe {

    PeScannerMgr::PeScannerMgr(IKittyMemOp* pMem)
    {
        m_pMem = pMem;

        const pid_t pid = m_pMem->processID();
        const std::string statusName = fatigue::getStatusName(pid);

        // KITTY_LOGI("Looking for process map of %s with pid %d", processName.c_str(), pid);

        // Find the correct map on this process that belongs to the actual executable.
        // We use the status name here because the cmdline path may contain unknown windows paths, but
        // the status name is the actual executable name, which should have an extension like .exe.
        // eg. a wine/proton process named "process.exe" should have a cmdline like "Z:\path\to\process.exe"
        // Other maps will have win/steam/proton etc paths, but only one should have the executable name
        std::vector<fatigue::ProcMap> maps = fatigue::getMapsEndWith(pid, statusName);
        for (auto &it : maps) {
            if (it.isValid() && !it.isUnknown()) {
                // Check if it's a valid Win32 PE executable
                // Read more than you want to at https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
                std::size_t bytesRead = 0;
                std::size_t currentOffset = 0;

                // Read DOS header, and check if it's a DOS executable
                struct DosHeader dosHeader = {0};
                bytesRead = m_pMem->Read(it.startAddress, &dosHeader, sizeof(DosHeader));
                if (bytesRead != sizeof(DosHeader)) {
                    KITTY_LOGE("Failed to read DOS header from region at %p for pid %ul", (void *)it.startAddress, pid);
                    continue;
                }

                if (dosHeader.magic != DOS_MAGIC) {
                    continue;
                }

                currentOffset = dosHeader.coff_header_offset;

                // Read COFF header, and check PE signature
                struct CoffHeader coffHeader = {0};
                bytesRead = m_pMem->Read(it.startAddress + currentOffset, &coffHeader, sizeof(CoffHeader));
                if (bytesRead != sizeof(CoffHeader)) {
                    KITTY_LOGE("Failed to read COFF header from region at %p for pid %ul", (void *)it.startAddress, pid);
                    continue;
                }

                if (coffHeader.signature != PE_SIGNATURE) {
                    continue;
                }

                currentOffset += sizeof(CoffHeader);

                // Read COFF optional header, and check PE32+ magic number
                std::size_t optionalHeaderSize = std::min(
                    // Do not read more than the size of the optional header, but also no more than the size of the struct
                    static_cast<std::size_t>(coffHeader.size_of_optional_header),
                    sizeof(CoffOptionalHeader)
                );
                struct CoffOptionalHeader coffOptionalHeader = {0};
                bytesRead = m_pMem->Read(it.startAddress + currentOffset, &coffOptionalHeader, optionalHeaderSize);
                if (bytesRead != optionalHeaderSize) {
                    KITTY_LOGE("Failed to read COFF optional header from region at %p for pid %ul", (void *)it.startAddress, pid);
                    continue;
                }

                if (coffOptionalHeader.magic != PE32PLUS_MAGIC || coffOptionalHeader.magic != PE32_MAGIC) {
                    continue;
                }

                currentOffset += coffHeader.size_of_optional_header;

                // Read section headers
                std::vector<SectionHeader> section_headers(coffHeader.number_of_sections);
                bytesRead = m_pMem->Read(it.startAddress + currentOffset, section_headers.data(), sizeof(SectionHeader) * coffHeader.number_of_sections);
                if (bytesRead != sizeof(SectionHeader) * coffHeader.number_of_sections) {
                    KITTY_LOGE("Failed to read section headers from region at %p for pid %ul", (void *)it.startAddress, pid);
                    continue;
                }

                // At this point, we have found the correct map
                m_procMap = new ProcMap(it);
                m_dosHeader = dosHeader;
                m_coffHeader = coffHeader;
                m_coffOptionalHeader = coffOptionalHeader;
                m_sectionHeaders = section_headers;
                break;
            }
        }
        // Done
    }

    void PeScannerMgr::dump()
    {
        printf("pe::ProcMap: %s", m_procMap->toString().c_str());
        printf("  DOS Header magic:      %s\n", fatigue::data2PrettyHex(m_dosHeader.magic).c_str());
        printf("  COFF Header signature: %s\n", fatigue::data2PrettyHex(m_coffHeader.signature).c_str());
        printf("  COFF Optional magic:   %s\n", fatigue::data2PrettyHex(m_coffOptionalHeader.magic).c_str());
        printf("  # of sections:         %d\n\n", m_coffHeader.number_of_sections);

        for (auto &section : m_sectionHeaders) {
            auto sectionName = std::string(section.name);
            // remove any non ascii characters (sometimes get weird characters at the end)
            sectionName = string::trim(sectionName);

            printf("    section: \"%s\"\n", sectionName.c_str());
            printf("      address: %d\n", section.virtual_address);
            printf("      size:    %d\n", section.virtual_size);
        }

        printf("\n");
    }

    SectionScanner* PeScannerMgr::getSection(const std::string &name)
    {
        SectionScanner *section = nullptr;

        for (auto &it : m_sectionHeaders) {
            // if secion name starts with a ., match with or without the dot
            if (name == it.name || (name[0] != '.' && name == std::string(it.name + 1))) {
                section = new SectionScanner(m_procMap, it);
                break;
            }
        }

        return section;
    }
}
