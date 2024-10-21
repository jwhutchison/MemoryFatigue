#include <format>
#include <string>
#include "fatigue.hpp"
#include "log.hpp"

using namespace fatigue;

void testLog() {
    logDebug("Debug message");
    logInfo("Info message");
    logSuccess("Success message");
    logFail("Fail message");
    logWarning("Warning message");
    logError("Error message");
}

int main(int argc, char* args[])
{
    // log::setLogFormat(log::LogFormat::Default);
    log::setLogFormat(log::LogFormat::Compact);
    // log::setLogFormat(log::LogFormat::Tiny);
    log::setLogLevel(log::LogLevel::Debug);

    // testLog();

    logInfo("Step 1: Find Sekiro.exe");

    std::string processName = "sekiro.exe";
    pid_t pid = getProcessIdByStatusName(processName);
    std::string statusName = getStatusName(pid);

    logInfo(std::format("Process Name: {}", processName.c_str()));
    logInfo(std::format("Process ID:   {}", pid));
    logInfo(std::format("Status Name:  {}", statusName.c_str()));

    // KITTY_LOGI("Located PE:   %s", process->peScanner.isValid() ? "Yes" : "No");

    // if (!process->peScanner.isValid()) {
    //     KITTY_LOGE("Failed to locate PE headers");
    //     return 1;
    // }

    logInfo("Step 2: Get process maps");

    // std::vector<Map> maps = getValidMaps(pid);
    // std::vector<Map> maps = getMaps(pid, processName);
    Map map = findMap(pid, processName);

    logInfo(map.toString());

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