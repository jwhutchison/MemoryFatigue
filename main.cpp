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
    // log::setLogFormat(log::LogFormat::Compact);
    log::setLogFormat(log::LogFormat::Tiny);
    log::setLogLevel(log::LogLevel::Debug);

    // testLog();

    logInfo("Step 0: Silly Billy String Stuff");

    std::string hex1 = "aabbccddeeff";
    std::string hex2 = "aa bb cc ?? ee ff";

    // search::Pattern pattern1 = search::parsePattern(hex1);
    // logInfo(std::format("Pattern 1: {}", hex1));
    // logInfo(std::format("Mask: {}, Data: \n{}", pattern1.mask, hex::dump(pattern1.bytes.data(), pattern1.bytes.size())));

    search::Pattern pattern2 = search::parsePattern(hex2);
    logInfo(std::format("Pattern 2: {}", hex2));
    logInfo(std::format("Mask: {}, Data: \n{}", pattern2.mask, hex::dump(pattern2.bytes.data(), pattern2.bytes.size())));


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


    logInfo("Step 2: Get process map");

    proc::Map map = proc::findMapEndsWith(pid, processName);
    // Map map = findMapEndsWith(pid, "dinput8.dll");
    logInfo(map.toString());


    logInfo("Step 3: Read memory");
    if (map.isValid()) {
        pe::DosHeader dos = {0};
        size_t bytesRead = 0;

        bytesRead = map.read(0, &dos);
        logInfo(std::format("read {} bytes at {:#x}", sizeof(dos), map.start));
        std::cout << hex::dump(&dos, sizeof(pe::DosHeader), 16) << std::endl;
    }

    logInfo("Step 4: Read PE headers");
    pe::PeMap peMap(map);

    if (!peMap.isValid()) {
        logError("Failed to read PE headers");
        return 1;
    }

    logInfo(std::format("DOS Header: {:#x}", peMap.dos().magic));
    logInfo(std::format("COFF Header: {:#x}", peMap.coff().signature));
    logInfo(std::format("Optional Header: {:#x}", peMap.optional().magic));

    // for (auto section : peMap.getSections()) {
    //     logInfo(std::format("Section: {}", section.name));
    // }

    Region textSection = peMap.getSection(".text");

    if (!textSection.isValid()) {
        logError("Failed to find .text section");
        return 1;
    }

    logInfo(std::format("Found text section: {} ({:#x}-{:#x})", textSection.name, textSection.start, textSection.end));

    unsigned char lol[0x100] = {0};
    textSection.read(0, &lol);
    std::cout << hex::dump(lol, sizeof(lol), 16) << std::endl;


    logInfo("Step 5: Search for pattern");

    logInfo("Searching for pattern: C7 43 ?? ?? ?? ?? ?? 4C 89 AB");
    std::vector<uintptr_t> results = textSection.find("C7 43 ?? ?? ?? ?? ?? 4C 89 AB");
    for (auto result : results) {
        logInfo(std::format("Found at {:#x}", result));
    }

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