#include "KittyMemoryMgr.hpp"
#include "Fatigue.hpp"
#include "utils.hpp"
#include "PE.hpp"

namespace fatigue::pe {

    Section* ProcMap::getSection(const std::string &name)
    {
        Section *section = nullptr;

        for (auto &it : section_headers) {
            // if secion name starts with a ., match with or without the dot
            if (name == it.name || (name[0] == '.' && name.substr(1) == it.name)) {
                section = new Section(this, it);
                break;
            }
        }

        return section;
    }

    ProcMap* getBaseProcessMap(KittyMemoryMgr &manager)
    {
        const pid_t pid = manager.processID();
        const std::string processName = fatigue::getProcessStatusName(pid);

        ProcMap *processMap = nullptr;

        // KITTY_LOGI("Looking for process map of %s with pid %d", processName.c_str(), pid);

        // Find the correct map on this process that belongs to the actual executable.
        // eg. a wine/proton process named "process.exe" should have a cmdline like "Z:\path\to\process.exe"
        // Other maps will have win/steam/proton etc paths, but only one should have the executable name
        std::vector<fatigue::ProcMap> maps = fatigue::getMapsEndWith(pid, processName);
        for (auto &it : maps) {
            if (it.isValid() && !it.isUnknown()) {
                // Check if it's a valid Win32 PE executable
                // Read more than you want to at https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
                std::size_t bytesRead = 0;
                std::size_t currentOffset = 0;

                // Read DOS header, and check if it's a DOS executable
                struct DosHeader dos_header = {0};
                bytesRead = manager.readMem(it.startAddress, &dos_header, sizeof(DosHeader));
                if (bytesRead != sizeof(DosHeader)) {
                    KITTY_LOGE("Failed to read DOS header from region at %p for pid %ul", (void *)it.startAddress, pid);
                    continue;
                }

                if (dos_header.magic != 0x5A4D) { // MZ
                    continue;
                }

                currentOffset = dos_header.coff_header_offset;

                // Read COFF header, and check PE signature
                struct CoffHeader coff_header = {0};
                bytesRead = manager.readMem(it.startAddress + currentOffset, &coff_header, sizeof(CoffHeader));
                if (bytesRead != sizeof(CoffHeader)) {
                    KITTY_LOGE("Failed to read COFF header from region at %p for pid %ul", (void *)it.startAddress, pid);
                    continue;
                }

                if (coff_header.signature != 0x00004550) { // PE\0\0
                    continue;
                }

                currentOffset += sizeof(CoffHeader);

                // Read COFF optional header, and check PE32+ magic number
                std::size_t optionalHeaderSize = std::min(
                    // Do not read more than the size of the optional header, but also no more than the size of the struct
                    static_cast<std::size_t>(coff_header.size_of_optional_header),
                    sizeof(CoffOptionalHeader)
                );
                struct CoffOptionalHeader coff_optional_header = {0};
                bytesRead = manager.readMem(it.startAddress + currentOffset, &coff_optional_header, optionalHeaderSize);
                if (bytesRead != optionalHeaderSize) {
                    KITTY_LOGE("Failed to read COFF optional header from region at %p for pid %ul", (void *)it.startAddress, pid);
                    continue;
                }

                if (coff_optional_header.magic != 0x20B) { // PE32+
                    continue;
                }

                currentOffset += coff_header.size_of_optional_header;

                // Read section headers
                std::vector<SectionHeader> section_headers(coff_header.number_of_sections);
                bytesRead = manager.readMem(it.startAddress + currentOffset, section_headers.data(), sizeof(SectionHeader) * coff_header.number_of_sections);
                if (bytesRead != sizeof(SectionHeader) * coff_header.number_of_sections) {
                    KITTY_LOGE("Failed to read section headers from region at %p for pid %ul", (void *)it.startAddress, pid);
                    continue;
                }

                // At this point, we have found the correct map
                processMap = new ProcMap(it);
                processMap->dos_header = dos_header;
                processMap->coff_header = coff_header;
                processMap->coff_optional_header = coff_optional_header;
                processMap->section_headers = section_headers;
                break;
            }
        }

        return processMap;
    }

    void ProcMap::dump()
    {
        printf("pe::ProcMap: %s", toString().c_str());
        printf("  DOS Header magic:      %s\n", fatigue::data2PrettyHex(dos_header.magic).c_str());
        printf("  COFF Header signature: %s\n", fatigue::data2PrettyHex(coff_header.signature).c_str());
        printf("  COFF Optional magic:   %s\n", fatigue::data2PrettyHex(coff_optional_header.magic).c_str());
        printf("  # of sections:         %d\n\n", coff_header.number_of_sections);

        for (auto &section : section_headers) {
            auto sectionName = std::string(section.name);
            // remove any non ascii characters (sometimes get weird characters at the end)
            sectionName.erase(std::find_if(sectionName.rbegin(), sectionName.rend(), [](unsigned char ch) {
                return std::isprint(ch);
            }).base(), sectionName.end());

            printf("    section: \"%s\"\n", sectionName.c_str());
            printf("      address: %d\n", section.virtual_address);
            printf("      size:    %d\n", section.virtual_size);
        }

        printf("\n");
    }
}
