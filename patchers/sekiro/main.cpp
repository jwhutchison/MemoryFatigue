#include <format>
#include <string>
#include <thread>
#include "fatigue.hpp"
#include "constants.hpp"

using namespace fatigue;

void loldump(Region region, uintptr_t address, std::string label, size_t size = 16)
{
    uint8_t buffer[size];
    ssize_t bytesRead = 0;
    bytesRead = region.read(address, buffer, sizeof(buffer), true);

    logInfo(label + ":");
    std::cout << hex::dump(buffer, bytesRead) << std::endl;
}

int main(int argc, char* args[])
{
    // log::setLogFormat(log::LogFormat::Default);
    // log::setLogFormat(log::LogFormat::Compact);
    log::setLogFormat(log::LogFormat::Tiny);
    log::setLogLevel(log::LogLevel::Info);

    // Use IO if available, otherwise use PTRACE
    mem::setAccessMethod(mem::AccessMethod::IO);

    std::string processName = "sekiro.exe";

    // Wait for process to start, for a Windows game, we are most interested in the .exe name
    pid_t pid = proc::waitForProcess([&processName]() {
        return proc::getProcessIdByStatusName(processName);
    }, 30);

    if (pid <= 0) {
        logError(std::format("Failed to find process '{}'", processName));
        return 1;
    }

    // delay to allow process to start
    proc::wait(1000);

    if(!proc::attach(pid)) {
        logError(std::format("Failed to attach to process {}", pid));
        return 1;
    }

    logInfo(std::format("Found and attached to {} ({})", processName, pid));

    // Find the correct map for the actual executable, and read the PE headers
    proc::Map map = proc::findMapEndsWith(pid, processName);
    pe::PeMap peMap(map);

    if (!peMap.isValid()) {
        logError("Failed to read PE headers");
        return 1;
    }

    // TODO: These are all hardcoded values, pull from args or config
    const int fps = 120;
    const sekiro::Resolution resolution = {
        .width = 3440,
        .height = 1440
    };

    Region text = peMap.getSection(".text");
    if (!text.isValid()) {
        logError("Failed to find .text section");
        return 1;
    }

    Region data = peMap.getSection(".data");
    if (!data.isValid()) {
        logError("Failed to find .data section");
        return 1;
    }

    // FPS
    // Framelock (fps delta) and speed fix must both be applied or not at all
    uintptr_t framelockAddress = text.findFirst(sekiro::PATTERN_FRAMELOCK_FUZZY);
    int framelockOffset = sekiro::PATTERN_FRAMELOCK_FUZZY_OFFSET;
    float framelockPatch = 1.0f / fps; // Framelock delta: frames per second -> seconds per frame

    uintptr_t speedFixAddress = text.findFirst(sekiro::PATTERN_FRAMELOCK_SPEED_FIX);
    int speedFixOffset = sekiro::PATTERN_FRAMELOCK_SPEED_FIX_OFFSET;
    float speedFixPatch = sekiro::findSpeedFixForRefreshRate(fps);

    if (framelockAddress && speedFixAddress) {
        logInfo(std::format("Patching FPS to {} (speed fix {})", fps, speedFixPatch));

        // Apply FPS
        text.write(framelockAddress + framelockOffset, &framelockPatch, sizeof(framelockPatch));

        // Apply speed fix
        // Credit to @Lahvuun for sekirofpsunlock for following
        // The pattern match + offset gives the address of an internal offset to the actual speed fix address, so read it
        uint32_t speedFixInternalOffset;
        text.read(speedFixAddress + speedFixOffset, &speedFixInternalOffset);
        // Then find the actual address by adding the offset value to the end of the offset value (+4 bytes)
        uint32_t speedFixPatchAddress = speedFixAddress + speedFixOffset + 4 + speedFixInternalOffset;
        // The actual address may be past the end of the .text section, so allow out of bounds write
        text.write(speedFixPatchAddress, &speedFixPatch, sizeof(speedFixPatch), true);

        logInfo("...Ok");
    } else {
        logError("Framelock or speed fix not found");
    }

    // Resolution
    // Width, height, and scaling fix (allow widescreen) must all be applied or not at all
    // Offsets are all 0
    uintptr_t resolutionAddress = data.findFirst(
        resolution.width < 1920
            ? sekiro::PATTERN_RESOLUTION_DEFAULT_720
            : sekiro::PATTERN_RESOLUTION_DEFAULT);

    uintptr_t scalingFixAddress = text.findFirst(sekiro::PATTERN_RESOLUTION_SCALING_FIX);
    std::vector<uint8_t> scalingFixPatch = hex::parse(sekiro::PATCH_RESOLUTION_SCALING_FIX_ENABLE);

    if (resolutionAddress && scalingFixAddress) {
        logInfo(std::format("Patching resolution to {}x{}", resolution.width, resolution.height));

        // Apply resolution
        data.write(resolutionAddress, &resolution, sizeof(resolution));

        // Apply resolution scaling fix
        text.write(scalingFixAddress, scalingFixPatch.data(), scalingFixPatch.size());

        logInfo("...Ok");
    } else {
        logError("Resolution or scaling fix not found");
    }

    // Disable Camera Reset
    uintptr_t cameraResetAddress = text.findFirst(sekiro::PATTERN_CAMRESET_LOCKON);
    int cameraResetOffset = sekiro::PATTERN_CAMRESET_LOCKON_OFFSET;
    uint8_t cameraResetPatch = sekiro::PATCH_CAMRESET_LOCKON_DISABLE;

    if (cameraResetAddress) {
        logInfo("Disabling camera reset");
        text.write(cameraResetAddress + cameraResetOffset, &cameraResetPatch, sizeof(cameraResetPatch));
        logInfo("...Ok");
    } else {
        logError("Camera reset not found");
    }

    // Enable Autoloot
    uintptr_t autolootAddress = text.findFirst(sekiro::PATTERN_AUTOLOOT);
    int autolootOffset = sekiro::PATTERN_AUTOLOOT_OFFSET;
    std::vector<uint8_t> autolootPatch = hex::parse(sekiro::PATCH_AUTOLOOT_ENABLE);

    if (autolootAddress) {
        logInfo("Enabling autoloot");
        text.write(autolootAddress + autolootOffset, autolootPatch.data(), autolootPatch.size());
        logInfo("...Ok");
    } else {
        logError("Autoloot not found");
    }

    // Done!
    logInfo("Done, enjoy!");
    proc::detach(pid);
    return 0;
}