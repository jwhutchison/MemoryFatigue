#include <format>
#include <string>
#include "fatigue.hpp"
#include "Patch.hpp"

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

    // IO access method is required for writing memory
    // You can set global access like this, or use SYS for reading and IO for writing (see below)
    // mem::setAccessMethod(mem::AccessMethod::IO);

    // testLog();

    logInfo("Step 1: Find process");

    std::string processName = "sekiro.exe";

    // Wait for a process to appear and get its PID using a lambda function
    pid_t pid = proc::waitForProcess([&processName]() {
        return proc::getProcessIdByStatusName(processName);
    }, 30);

    // Other ways to get the PID
    // pid_t pid = proc::getProcessIdByStatusName(processName);
    // pid_t pid = proc::getProcessIdByCmdlineEndsWith(processName);
    // pid_t pid = proc::getProcessIdByCmdlineContains(processName);

    if (pid <= 0) {
        logError(std::format("Failed to find process '{}'", processName));
        return 1;
    }

    // Learn things about the process
    std::string statusName = proc::getStatusName(pid);
    std::string cmdline = proc::getCmdline(pid);

    logInfo(std::format("Input Name:   {}", processName));
    logInfo(std::format("Process ID:   {}", pid));
    logInfo(std::format("Status Name:  {}", statusName));
    logInfo(std::format("Cmdline:      {}", cmdline));


    logInfo("Step 2: Get process map");

    logInfo("Step 2.1: Get a map for an Elf file (skipped)");

    // proc::Map soMap = proc::findMapEndsWith(pid, "steamoverlayvulkanlayer.so");

    // if (soMap.isValid()) {
    //     elf::ElfHeader elfhead = {0};
    //     soMap.read(0, &elfhead);
    //     logInfo(std::format("Elf Header: {:#x}", elfhead.magic));
    //     std::cout << hex::dump(&elfhead, sizeof(elf::ElfHeader), soMap.start);
    // }

    // elf::ElfMap elfMap(soMap);
    // logInfo(std::format("Elf Map {}", elfMap.toString()));

    logInfo("Step 2.2: Get map for the EXE");

    // We want to find the process map for Sekiro.exe out of all the memory maps
    // loaded for the sekiro.exe process
    proc::Map map = proc::findMapEndsWith(pid, processName);
    logInfo(map.toString());

    // It's possible to find maps for DLLs loaded for the process as well
    // Map map = findMapEndsWith(pid, "dinput8.dll");

    // Once we have the map, we can read from it directly
    logInfo("Step 2.5: Demo a memory read");

    if (map.isValid()) {
        pe::DosHeader dos = {0};
        map.read(0, &dos);
        logInfo(std::format("DOS Header: {:#x}", dos.magic));
        std::cout << hex::dump(&dos, sizeof(pe::DosHeader), map.start) << std::endl;
    }


    logInfo("Step 3: Read PE headers");

    // For a PE file, we can use PeMap to read the PE headers
    pe::PeMap peMap(map);

    if (!peMap.isValid()) {
        logError("Failed to read PE headers");
        return 1;
    }

    logInfo(std::format("DOS Header: {:#x}", peMap.dos().magic));
    logInfo(std::format("COFF Header: {:#x}", peMap.coff().signature));
    logInfo(std::format("Optional Header: {:#x}", peMap.optional().magic));

    // Sections are where the code we want to read is located
    // for (auto section : peMap.getSections()) {
    //     logInfo(std::format("Section: {}", section.name));
    // }


    logInfo("Step 4: Get .text section and search for pattern");

    Region textSection = peMap.getSection(".text");

    if (!textSection.isValid()) {
        logError("Failed to find .text section");
        return 1;
    }

    logInfo(std::format("Found text section: {} ({:#x}-{:#x})", textSection.name, textSection.start, textSection.end));

    // Search for a pattern in the .text section using a string
    // logInfo("Searching for pattern: C7 43 ?? ?? ?? ?? ?? 4C 89 AB");
    // std::vector<uintptr_t> results = textSection.find("C7 43 ?? ?? ?? ?? ?? 4C 89 AB");
    // for (auto result : results) {
    //     logInfo(std::format("Found at {:#x}", result));
    // }


    logInfo("Step 5: Attach to process");

    // In order to write to memory, we need to attach to the process to pause it
    proc::attach(pid);
    // And then set to IO access method (or PTRACE) for writing
    textSection.method = mem::AccessMethod::IO;


    logInfo("Step 6: Apply a patch");

    // logInfo("Step 6.1: Manual patch");

    // logInfo("Searching for pattern: C6 86 ?? ?? 00 00 ?? F3 0F 10 8E ?? ?? 00 00");
    // uintptr_t found = textSection.findFirst("C6 86 ?? ?? 00 00 ?? F3 0F 10 8E ?? ?? 00 00");

    // if (found) {
    //     logInfo(std::format("Found at {:#x}", found));

    //     unsigned char buffer[16] = {0};
    //     ssize_t bytesRead = 0;
    //     ssize_t bytesWritten = 0;

    //     // before patch
    //     bytesRead = textSection.read(found, buffer, sizeof(buffer));
    //     logInfo("Original bytes:");
    //     std::cout << hex::dump(buffer, bytesRead) << std::endl;

    //     // Patch the byte at the found address
    //     int offset = 6;
    //     unsigned char patch = 0x00;

    //     bytesWritten = textSection.write(found + offset, &patch, 1);
    //     logInfo(std::format("Wrote {} bytes to {:#x} + {}", bytesWritten, found, offset));

    //     // After patch
    //     bytesRead = textSection.read(found, buffer, sizeof(buffer));
    //     logInfo("After patch:");
    //     std::cout << hex::dump(buffer, bytesRead) << std::endl;

    //     // Restore the original byte
    //     patch = 0x01;
    //     bytesWritten = textSection.write(found + offset, &patch, 1);
    //     logInfo(std::format("Restored {} bytes to {:#x} + {}", bytesWritten, found, offset));

    //     // After restore
    //     bytesRead = textSection.read(found, buffer, sizeof(buffer));
    //     logInfo("After restore:");
    //     std::cout << hex::dump(buffer, bytesRead) << std::endl;

    // } else {
    //     logError("Pattern not found");
    // }

    // logInfo("Step 6.2: Using Patch class");

    // Patch patch(textSection, "C6 86 ?? ?? 00 00 ?? F3 0F 10 8E ?? ?? 00 00", 6, "01");

    // if (!patch.isValid()) {
    //     logError("Patch is invalid");
    //     return 1;
    // }

    // logInfo(patch.dump());

    // // Apply the patch
    // if (patch.apply()) {
    //     logInfo("Patch applied");
    // } else {
    //     logError("Failed to apply patch");
    // }

    // logInfo(patch.dumpPatch());

    // // Restore the patch
    // if (patch.restore()) {
    //     logInfo("Patch restored");
    // } else {
    //     logError("Failed to restore patch");
    // }

    // logInfo(patch.dumpPatch());

    // // Do a not-found patch
    // Patch badPatch(textSection, "FF 01 ?? ?? 04 04 06 07 08 09 10", 2, "01");

    // if (!badPatch.isValid()) {
    //     logError("Bad patch is invalid (expected error)");
    // }

    // logInfo(badPatch.dump());

    // // Do a multi-found patch
    // Patch multiPatch(textSection, "00 00 ?? 00 00", 2, "01");

    // if (!multiPatch.isValid()) {
    //     logError("Multi patch is invalid");
    // }

    // logInfo(multiPatch.dump());


    logInfo("Done! Detach from process");

    proc::detach(pid);

    return 0;
}