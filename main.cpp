#include <format>
#include <string>
#include "fatigue.hpp"

// #include "KittyMemoryEx/KittyMemoryMgr.hpp"

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

    pid_t pid = proc::waitForProcess([&processName]() {
        return proc::getProcessIdByStatusName(processName);
    }, 30);

    // pid_t pid = proc::getProcessIdByStatusName(processName);
    // pid_t pid = proc::getProcessIdByCmdlineEndsWith(processName);
    // pid_t pid = proc::getProcessIdByCmdlineContains(processName);

    if (pid <= 0) {
        logError(std::format("Failed to find process '{}'", processName));
        return 1;
    }

    std::string statusName = proc::getStatusName(pid);
    std::string cmdline = proc::getCmdline(pid);

    logInfo(std::format("Input Name:   {}", processName));
    logInfo(std::format("Process ID:   {}", pid));
    logInfo(std::format("Status Name:  {}", statusName));
    logInfo(std::format("Cmdline:      {}", cmdline));

    // KITTY_LOGI("Located PE:   %s", process->peScanner.isValid() ? "Yes" : "No");

    // if (!process->peScanner.isValid()) {
    //     KITTY_LOGE("Failed to locate PE headers");
    //     return 1;
    // }

    logInfo("Step 2: Get process map");

    // std::vector<proc::Map> maps = proc::getMaps(pid);
    // std::vector<proc::Map> maps = proc::getValidMaps(pid);
    // std::vector<proc::Map> maps = proc::getMapsEndsWith(pid, ".exe");
    // for (auto map : maps) {
    //     logInfo(map.toString());
    // }
    // proc::Map map = maps.front();

    proc::Map map = proc::findMapEndsWith(pid, processName);
    // Map map = findMapEndsWith(pid, "dinput8.dll");

    logInfo(map.toString());

    logInfo("Step 3: Read memory");
    if (map.isValid()) {

        pe::DosHeader lol = {0};
        size_t bytesRead = 0;

        // bytesRead = mem::sys::read(map.pid, map.start, &lol, sizeof(lol));
        // bytesRead = mem::sys::read(map.pid, map.start, &lol);
        // bytesRead = region.read(0, &lol, sizeof(lol));
        bytesRead = map.read(0, &lol);
        logInfo(std::format("Fatigue Read2 {} bytes at {:#x}", sizeof(lol), map.start));
        std::cout << hex::dump(&lol, sizeof(pe::DosHeader), 16) << std::endl;
    }

    logInfo("Step 4: Read PE headers");
    pe::PeMap peMap(map);
    if (peMap.isValid()) {

        logInfo(std::format("DOS Header: {:#x}", peMap.dos().magic));
        logInfo(std::format("COFF Header: {:#x}", peMap.coff().signature));
        logInfo(std::format("Optional Header: {:#x}", peMap.optional().magic));

        for (auto section : peMap.sections()) {
            logInfo(std::format("Section: {}", section.name));
        }
    }

    // std::string haystack = "ABCDEFGABCDABCDABC";
    // std::string needle = "ABCDAB";

    // std::vector<uintptr_t> found = search::search(haystack.c_str(), haystack.size(), needle.c_str(), needle.size(), "", false);

    // for (auto f : found) {
    //     logInfo(std::format("Found {} at offset {}", needle, f));
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