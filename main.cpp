#include "Fatigue.hpp"
#include "KittyMemoryMgr.hpp"
#include "utils.hpp"
#include "PE.hpp"
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

using namespace fatigue;

KittyMemoryMgr manager;

int main(int argc, char* args[])
{
    KITTY_LOGI("================ FIND PROCESS BY SHORT NAME ===============");

    std::string processName = "sekiro.exe";

    // get process ID
    pid_t processID = getProcessIDByStatusName(processName);
    if(!processID) {
        KITTY_LOGI("Couldn't find process id for %s.", processName.c_str());
        return 1;
    }

    std::string processCmd = getProcessName(processID);

    KITTY_LOGI("Process Name: %s", processName.c_str());
    KITTY_LOGI("Process ID:   %d", processID);
    KITTY_LOGI("Process Cmd:  %s", processCmd.c_str());

    // initialize KittyMemoryMgr instance with process ID
    if(!manager.initialize(processID, EK_MEM_OP_SYSCALL, true)) {
        KITTY_LOGI("Error occurred )':");
        return 1;
    }

    KITTY_LOGI("================ GET MAP ===============");

    pe::ProcMap* processMap = pe::getBaseProcessMap(manager);
    pe::Section* textSection = nullptr;

    if(processMap) {
        KITTY_LOGI("Found map: %s", processMap->toString().c_str());
        textSection = processMap->getSection(".text");
    }

    KITTY_LOGI("================ FIND TEXT SECTION ===============");

    if(textSection && textSection->isValid()) {
        KITTY_LOGI("Found text section: \"%s\"",
                   textSection->getName().c_str());

        uintptr_t search_start = textSection->getStartAddress();
        uintptr_t search_end = textSection->getEndAddress();

        KITTY_LOGI("search start %p", (void*)search_start);
        KITTY_LOGI("search end %p", (void*)search_end);

        // scan with ida pattern & get one result
        uintptr_t found_at = 0;

        found_at = manager.memScanner.findIdaPatternFirst(
            search_start, search_end, "C7 43 ? ? ? ? ? 4C 89 AB");
        if(found_at) {
            KITTY_LOGI("found ida pattern at %p in %s", (void*)found_at,
                       textSection->getName().c_str());

            char buffer[10] = {0};
            int offset = 0x0;
            std::size_t bytesRead
                = manager.readMem(found_at + offset, buffer, sizeof(buffer));
            printf("  %s\n", fatigue::data2PrettyHex(buffer).c_str());
            KITTY_LOGI("\n%s",
                       KittyUtils::HexDump(buffer, sizeof(buffer)).c_str());
        }
    }

    delete processMap;
    delete textSection;

    return 0;
}