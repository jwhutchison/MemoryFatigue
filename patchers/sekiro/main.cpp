#include <format>
#include <string>
#include <thread>
#include <tclap/CmdLine.h>
#include "fatigue.hpp"
#include "constants.hpp"

using namespace fatigue;

struct options {
    int fps = -1;
    sekiro::Resolution resolution { -1, -1 };
    bool cameraReset = false;
    bool autoloot = false;
    bool verbose = false;
    int timeout = -1;
    int wait = -1;
};

options getArgs(int argc, char* args[]) {
    try {
        // Define the command line parser
        TCLAP::CmdLine cmd("Sekiro: Shadows Die Twice Patcher", ' ', "1.0");
        TCLAP::ValueArg<int> fpsArg("f", "fps", "Unlock FPS max", false, -1, "int", cmd);
        TCLAP::ValueArg<std::string> resolutionArg("r", "resolution", "Game resolution (WxH), e.g. '3440x1440'", false, "", "string", cmd);
        TCLAP::SwitchArg cameraResetArg("c", "no-camera-reset", "Disable camera reset on lock-on when no target", cmd);
        TCLAP::SwitchArg autolootArg("a", "autoloot", "Enable autoloot", cmd);

        TCLAP::SwitchArg verboseArg("v", "verbose", "Verbose output", cmd);
        TCLAP::ValueArg<int> timeoutArg("t", "timeout", "Seconds to wait for game to start", false, 30, "int", cmd);
        TCLAP::ValueArg<int> waitArg("w", "wait", "Milliseconds to wait after game starts (increase if getting errors on start)", false, 1000, "int", cmd);

        cmd.parse(argc, args);

        // Parse arguments
        options opts;

        opts.fps = fpsArg.getValue();
        if (opts.fps < 30 || opts.fps > 300) {
            throw std::runtime_error("FPS must be between 30 and 300");
        }

        std::string resolutionString = resolutionArg.getValue();
        if (!resolutionString.empty()) {
            auto pos = resolutionString.find('x');
            if (pos != std::string::npos) {
                opts.resolution.width = std::stoi(resolutionString.substr(0, pos));
                opts.resolution.height = std::stoi(resolutionString.substr(pos + 1));
            } else {
                throw std::runtime_error("Resolution must be WxH, e.g. '3440x1440'");
            }
        }

        opts.cameraReset = cameraResetArg.getValue();
        opts.autoloot = autolootArg.getValue();

        opts.verbose = verboseArg.getValue();
        opts.timeout = timeoutArg.getValue();
        opts.wait = waitArg.getValue();

        return opts;
    } catch (TCLAP::ArgException &e) {
        std::string error = std::format("Error: {} for argument {}", e.error(), e.argId());
        throw std::runtime_error(error);
    }
}

int main(int argc, char* args[])
{
    // Parse args and complain if invalid
    options opts;
    try {
        opts = getArgs(argc, args);
    } catch (std::runtime_error &e) {
        logError(e.what());
        return 1;
    }

    // log::setLogFormat(log::LogFormat::Default);
    // log::setLogFormat(log::LogFormat::Compact);
    log::setLogFormat(log::LogFormat::Tiny);
    log::setLogLevel(opts.verbose ? log::LogLevel::Info : log::LogLevel::Warning);

    // Use IO if available, otherwise use PTRACE
    mem::setAccessMethod(mem::AccessMethod::IO);

    std::string processName = "sekiro.exe";

    // Wait for process to start, for a Windows game, we are most interested in the .exe name
    pid_t pid = proc::waitForProcess([&processName]() {
        return proc::getProcessIdByStatusName(processName);
    }, opts.timeout);

    if (pid <= 0) {
        logError(std::format("Failed to find process '{}'", processName));
        return 1;
    }

    // delay to allow process to start
    proc::wait(opts.wait);

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

    Region text = peMap.getSection(".text");
    text.enforceBounds = false; // Allow out of bounds write (speed fix points outside .text)
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
    float framelockPatch = 1.0f / opts.fps; // Framelock delta: frames per second -> seconds per frame

    uintptr_t speedFixAddress = text.findFirst(sekiro::PATTERN_FRAMELOCK_SPEED_FIX);
    int speedFixOffset = sekiro::PATTERN_FRAMELOCK_SPEED_FIX_OFFSET;
    float speedFixPatch = sekiro::findSpeedFixForRefreshRate(opts.fps);

    if (framelockAddress && speedFixAddress) {
        logInfo(std::format("Patching FPS to {} (speed fix {})", opts.fps, speedFixPatch));

        // Apply FPS
        text.write(framelockAddress + framelockOffset, &framelockPatch, sizeof(framelockPatch));

        // Apply speed fix
        // Credit to @Lahvuun for sekirofpsunlock for the following
        // The pattern match + offset gives the address of an internal offset to the actual speed fix address, so read it
        uint32_t speedFixInternalOffset;
        text.read(speedFixAddress + speedFixOffset, &speedFixInternalOffset);
        // Then find the actual address by adding the offset value to the end of the offset value (+4 bytes)
        uint32_t speedFixPatchAddress = speedFixAddress + speedFixOffset + 4 + speedFixInternalOffset;
        // The actual address may be past the end of the .text section, make sure section bounds are not enforced
        text.write(speedFixPatchAddress, &speedFixPatch, sizeof(speedFixPatch));

        logInfo("...Ok");
    } else {
        logError("Framelock or speed fix not found");
    }

    // Resolution
    // Width, height, and scaling fix (allow widescreen) must all be applied or not at all
    // Offsets are all 0
    uintptr_t resolutionAddress = data.findFirst(
        opts.resolution.width < 1920
            ? sekiro::PATTERN_RESOLUTION_DEFAULT_720
            : sekiro::PATTERN_RESOLUTION_DEFAULT);

    uintptr_t scalingFixAddress = text.findFirst(sekiro::PATTERN_RESOLUTION_SCALING_FIX);
    std::vector<uint8_t> scalingFixPatch = hex::parse(sekiro::PATCH_RESOLUTION_SCALING_FIX_ENABLE);

    if (resolutionAddress && scalingFixAddress) {
        logInfo(std::format("Patching resolution to {}x{}", opts.resolution.width, opts.resolution.height));

        // Apply resolution
        data.write(resolutionAddress, &opts.resolution, sizeof(opts.resolution));

        // Apply resolution scaling fix
        text.write(scalingFixAddress, scalingFixPatch.data(), scalingFixPatch.size());

        logInfo("...Ok");
    } else {
        logError("Resolution or scaling fix not found");
    }

    // Disable Camera Reset
    if (opts.cameraReset) {
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
    }

    // Enable Autoloot
    if (opts.autoloot) {
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
    }

    // Done!
    logInfo("Done, enjoy!");
    proc::detach(pid);
    return 0;
}