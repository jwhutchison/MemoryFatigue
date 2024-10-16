#define FATIGUE_LOG_LEVEL fatigue::log::LogLevel::Debug
#define FATIGUE_LOG_COMPACT false
#define FATIGUE_COLOR true

#include "log.hpp"
#include "memory.hpp"
#include <cstdint>
#include <format>
#include <string>
#include <thread>
#include <vector>

using namespace fatigue;

int main(int argc, char* args[])
{
    logInfo("==>> Starting Fatigue");

    std::string processName = "sekiro.exe";

    // Attach to process
    // Fatigue *process = Fatigue::attach(processName, &getProcessIDByStatusName);

    logDebug(std::format("Process Name: {}", processName.c_str()));

    logFail("This is a fail message");
    logSuccess("This is a success message");

    logWarning("This isn't implemented yet");
    logError("No really, it's not implemented");

    // KITTY_LOGI("Process ID:   %d", process->processID());
    // KITTY_LOGI("Process Cmd:  %s", process->processName().c_str());
    // KITTY_LOGI("Located PE:   %s", process->peScanner.isValid() ? "Yes" : "No");

    // if (!process->peScanner.isValid()) {
    //     KITTY_LOGE("Failed to locate PE headers");
    //     return 1;
    // }

    // KITTY_LOGI("Found map: %s", process->peScanner.getProcMap()->toString().c_str());

    // pe::SectionScanner *textSection = process->getPeSection(".text");

    // if(textSection && textSection->isValid()) {
    //     KITTY_LOGI("Found text section: \"%s\"",
    //                textSection->getName().c_str());

    //     uintptr_t search_start = textSection->getStartAddress();
    //     uintptr_t search_end = textSection->getEndAddress();

    //     KITTY_LOGI("search start %p", (void*)search_start);
    //     KITTY_LOGI("search end %p", (void*)search_end);

    //     // scan with ida pattern & get one result
    //     uintptr_t found_at = 0;

    //     found_at = process->memScanner.findIdaPatternFirst(
    //         search_start, search_end, "C7 43 ? ? ? ? ? 4C 89 AB");
    //     if(found_at) {
    //         KITTY_LOGI("found ida pattern at %p in %s", (void*)found_at,
    //                    textSection->getName().c_str());

    //         char buffer[10] = {0};
    //         int offset = 0x0;
    //         std::size_t bytesRead
    //             = process->readMem(found_at + offset, buffer, sizeof(buffer));
    //         printf("  %s\n", fatigue::data2PrettyHex(buffer).c_str());
    //         KITTY_LOGI("\n%s",
    //                    KittyUtils::HexDump(buffer, sizeof(buffer)).c_str());
    //     }
    // }

    return 0;
}