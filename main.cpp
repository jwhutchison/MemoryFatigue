#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include "../KittyMemoryEx/KittyMemoryEx/KittyMemoryMgr.hpp"
#include "common.h"

KittyMemoryMgr kittyMemMgr;

template <typename T>
void printAsHex(const T &data)
{
    const auto *byteData = reinterpret_cast<const unsigned char *>(&data);
    for (int i = 0; i < sizeof(T); i++) {
        printf("%02X ", byteData[i]);
    }
}

std::string getProcessStatusName(pid_t pid)
{
    if (pid <= 0)
        return "";

    char status[256] = {0};
    snprintf(status, sizeof(status), "/proc/%d/status", pid);

    errno = 0;
    FILE *fp = fopen(status, "r");
    if (!fp)
    {
        KITTY_LOGE("Couldn't open status file %s, error=%s", status, strerror(errno));
        return "";
    }

    std::string name = "";

    char line[128] = {0};
    while (fgets(line, sizeof(line), fp)) {
        if (KittyUtils::String::StartsWith(line, "Name:")) {
            name = line + 5;
            // TODO: Move to util
            name.erase(name.begin(), std::find_if(name.begin(), name.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
            name.erase(std::find_if(name.rbegin(), name.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), name.end());
            break;
        }
    }

    fclose(fp);
    return name;
}

pid_t getProcessIDByStatusName(const std::string &processName)
{
    if (processName.empty())
        return 0;

    pid_t pid = 0;

    errno = 0;
    DIR *dir = opendir("/proc/");
    if (!dir)
    {
        KITTY_LOGE("Couldn't open /proc/, error=%s", strerror(errno));
        return pid;
    }

    dirent *entry = nullptr;
    while ((entry = readdir(dir)) != nullptr)
    {
        int entry_pid = atoi(entry->d_name);
        if (entry_pid > 0)
        {
            if (processName == getProcessStatusName(entry_pid))
            {
                pid = entry_pid;
                break;
            }
        }
    }
    closedir(dir);
    return pid;
}

int main(int argc, char *args[])
{
    KITTY_LOGI("================ FIND PROCESS BY SHORT NAME ===============");

    std::string processName = "sekiro.exe";

    // get process ID
    pid_t processID = getProcessIDByStatusName(processName);
    if (!processID)
    {
        KITTY_LOGI("Couldn't find process id for %s.", processName.c_str());
        return 1;
    }

    std::string processCmd = KittyMemoryEx::getProcessName(processID);

    KITTY_LOGI("Process Name: %s", processName.c_str());
    KITTY_LOGI("Process ID:   %d", processID);
    KITTY_LOGI("Process Cmd:  %s", processCmd.c_str());

    // initialize KittyMemoryMgr instance with process ID
    if (!kittyMemMgr.initialize(processID, EK_MEM_OP_SYSCALL, true))
    {
        KITTY_LOGI("Error occurred )':");
        return 1;
    }

    KITTY_LOGI("================ GET MAP ===============");

    std::vector<ProcMap> maps = KittyMemoryEx::getAllMaps(processID);

    // Find map, get section by name
    ProcMap *processMap = nullptr;
    section_header textSection = {0};

    for (auto &it : maps) {
        // Find the correct map
        if (it.isValid() && !it.isUnknown()) {
            if (!KittyUtils::String::EndsWith(it.pathname, processName)) {
                // printf("NOT %s Map: %s\n", processName.c_str(), it.toString().c_str());
                continue;
            }

            std::size_t bytesRead = 0;
            std::size_t currentOffset = 0;

            // Read DOS header, and check if it's a DOS executable
            struct dos_header dos_header = {0};
            bytesRead = kittyMemMgr.readMem(it.startAddress, &dos_header, sizeof(dos_header));
            if (bytesRead != sizeof(dos_header)) {
                // KITTY_LOGE("Failed to read DOS header from region at %p", (void *)it.startAddress);
                continue;
            }

            if (dos_header.magic != 0x5A4D) { // MZ
                continue;
            }

            currentOffset = dos_header.coff_header_offset;

            // Read COFF header, and check PE signature
            struct coff_header coff_header = {0};
            bytesRead = kittyMemMgr.readMem(it.startAddress + currentOffset, &coff_header, sizeof(coff_header));
            if (bytesRead != sizeof(coff_header)) {
                KITTY_LOGE("Failed to read COFF header from region at %p", (void *)(it.startAddress));
                continue;
            }

            if (coff_header.signature != 0x00004550) { // PE\0\0
                continue;
            }

            currentOffset += sizeof(coff_header);

            // Read COFF optional header, and check if it's a 32-bit executable
            struct coff_optional_header coff_optional_header = {0};
            bytesRead = kittyMemMgr.readMem(it.startAddress + currentOffset, &coff_optional_header, coff_header.size_of_optional_header);
            if (bytesRead != coff_header.size_of_optional_header) {
                KITTY_LOGE("Failed to read COFF optional header from region at %p", (void *)(it.startAddress));
                continue;
            }

            if (coff_optional_header.magic != 0x20B) { // PE32+
                continue;
            }

            currentOffset += coff_header.size_of_optional_header;

            // Read section headers
            std::vector<section_header> section_headers(coff_header.number_of_sections);
            bytesRead = kittyMemMgr.readMem(it.startAddress + currentOffset, section_headers.data(), sizeof(section_header) * coff_header.number_of_sections);

            if (bytesRead != sizeof(section_header) * coff_header.number_of_sections) {
                KITTY_LOGE("Failed to read section headers from region at %p", (void *)(it.startAddress));
                continue;
            }

            for (auto &section : section_headers) {
                // if name is .text
                auto sectionName = std::string(section.name);
                // remove any non ascii characters (sometimes get weird characters at the end)
                sectionName.erase(std::find_if(sectionName.rbegin(), sectionName.rend(), [](unsigned char ch) {
                    return std::isprint(ch);
                }).base(), sectionName.end());

                if (sectionName == ".text") {
                    printf("Found .text section: \"%s\"\n", section.name);
                    textSection = section;
                    break;
                }

            }

            // At this point, we have found the correct map
            processMap = new ProcMap(it);

            // KITTY_LOGI("Map: %s", it.toString().c_str());
            // // printf("  bytesRead: %lu\n", bytesRead);
            // // KITTY_LOGI("magic: %s", KittyUtils::data2Hex(magic).c_str());

            // printf("  magic: ");
            // printAsHex(dos_header.magic);
            // printf("\n");

            // printf("  coff_header_signature: ");
            // printAsHex(coff_header.signature);
            // printf("\n");

            // printf("  coff_optional_header_magic: ");
            // printAsHex(coff_optional_header.magic);
            // printf("\n");

            // printf("  number of sections: %d\n", coff_header.number_of_sections);
            // // KITTY_LOGI("\n%s", KittyUtils::HexDump(magic, sizeof(magic)).c_str());

            // for (auto &section : section_headers) {
            //     auto sectionName = std::string(section.name);
            //     // remove any non ascii characters (sometimes get weird characters at the end)
            //     sectionName.erase(std::find_if(sectionName.rbegin(), sectionName.rend(), [](unsigned char ch) {
            //         return std::isprint(ch);
            //     }).base(), sectionName.end());

            //     printf("    section: \"%s\"\n", sectionName);
            //     printf("      virtual_size: %d\n", section.virtual_size);
            //     printf("      virtual_address: %d\n", section.virtual_address);
            // }

            // printf("\n");
        }
    }

    if (processMap) {
        KITTY_LOGI("Found map: %s\n", processMap->toString().c_str());
    }

    if (processMap && textSection.virtual_address > 0) {
        KITTY_LOGI("Found text section: \"%s\"\n", textSection.name);

        uintptr_t search_start = processMap->startAddress + textSection.virtual_address;
        uintptr_t search_end = processMap->startAddress + textSection.virtual_address + textSection.virtual_size;

        KITTY_LOGI("search start %p", (void*)search_start);
        KITTY_LOGI("search end %p", (void*)search_end);

        // scan with ida pattern & get one result
        uintptr_t found_at = 0;

        found_at = kittyMemMgr.memScanner.findIdaPatternFirst(search_start, search_end, "C7 43 ? ? ? ? ? 4C 89 AB");
        if (found_at)
        {
            KITTY_LOGI("found ida pattern at %p in %s", (void *)found_at, textSection.name);

            char buffer[10] = {0};
            int offset = 0x0;
            std::size_t bytesRead = kittyMemMgr.readMem(found_at + offset, buffer, sizeof(buffer));
            printf("  ");
            printAsHex(buffer);
            printf("\n");
            KITTY_LOGI("\n%s", KittyUtils::HexDump(buffer, sizeof(buffer)).c_str());
        }

    }

    delete processMap;

    return 0;
}