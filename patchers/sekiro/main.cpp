#include <format>
#include <string>
#include <thread>
#include <tclap/CmdLine.h>
#include "fatigue.hpp"
#include "constants.hpp"

using namespace fatigue;

// Define and parse command line arguments

struct options {
    int fps = -1;
    sekiro::Resolution resolution { -1, -1 };
    bool cameraReset = false;
    bool autoloot = false;
    bool verbose = false;
    int timeout = -1;
    int delay = -1;
};

options parseArgs(int argc, char* args[]) {
    try {
        // Define the command line parser
        TCLAP::CmdLine cmd("Sekiro: Shadows Die Twice Patcher", ' ', "1.0");
        TCLAP::ValueArg<int> fpsArg("f", "fps", "Set game FPS (between 30 and 300)", false, -1, "int", cmd);
        TCLAP::ValueArg<std::string> resolutionArg("r", "resolution", "Game resolution (WxH), e.g. '3440x1440'", false, "", "string", cmd);
        TCLAP::SwitchArg cameraResetArg("c", "no-camera-reset", "Disable camera reset on lock-on when no target", cmd);
        TCLAP::SwitchArg autolootArg("a", "autoloot", "Enable autoloot", cmd);

        TCLAP::SwitchArg verboseArg("V", "verbose", "Verbose output", cmd);
        TCLAP::ValueArg<int> timeoutArg("T", "timeout", "Seconds to wait for game to start", false, 30, "int", cmd);
        TCLAP::ValueArg<int> delayArg("D", "delay", "Milliseconds to wait after game starts (increase if errors on start)", false, 1000, "int", cmd);

        cmd.parse(argc, args);

        // Borrow the TCLAP output handler so we can format our bounds errors
        TCLAP::StdOutput out;

        // Populate and validate arguments
        options opts;

        opts.fps = fpsArg.getValue();
        if (opts.fps != -1 && (opts.fps < 30 || opts.fps > 300)) {
            TCLAP::ArgException err("FPS must be between 30 and 300", "fps");
            out.failure(cmd, err);
        }

        std::string resolutionString = resolutionArg.getValue();
        if (!resolutionString.empty()) {
            try {
                auto pos = resolutionString.find('x');
                if (pos != std::string::npos) {
                    opts.resolution.width = std::stoi(resolutionString.substr(0, pos));
                    opts.resolution.height = std::stoi(resolutionString.substr(pos + 1));
                } else {
                    throw "invalid";
                }
            } catch (...) {
                TCLAP::ArgException err("Invalid resolution '" + resolutionString + "', please format as WxH, e.g. '3440x1440'", "resolution");
                out.failure(cmd, err);
            }
        }

        opts.cameraReset = cameraResetArg.getValue();
        opts.autoloot = autolootArg.getValue();

        opts.verbose = verboseArg.getValue();
        opts.timeout = timeoutArg.getValue();
        opts.delay = delayArg.getValue();

        return opts;
    } catch (const TCLAP::ArgException &e) {
        std::cerr << e.error() << " for arg " << e.argId() << std::endl;
        exit(1);
    } catch (const TCLAP::ExitException &e) {
        exit(e.getExitStatus());
    }
}

// Patchers

bool patchFps(Region const &text, Region const &data, int fps)
{
    // Framelock (fps delta) and speed fix must both be applied or not at all
    uintptr_t framelockAddress = text.findFirst(sekiro::PATTERN_FRAMELOCK_FUZZY);
    int framelockOffset = sekiro::PATTERN_FRAMELOCK_FUZZY_OFFSET;
    float framelockPatch = 1.0f / fps; // Framelock delta: frames per second -> seconds per frame

    uintptr_t speedFixAddress = text.findFirst(sekiro::PATTERN_FRAMELOCK_SPEED_FIX);
    int speedFixOffset = sekiro::PATTERN_FRAMELOCK_SPEED_FIX_OFFSET;
    float speedFixPatch = sekiro::findSpeedFixForRefreshRate(fps);

    if (!framelockAddress || !speedFixAddress) {
        logWarning("Framelock or speed fix not found");
        return false;
    }

    logInfo(std::format("Patching FPS to {} (speed fix {})", fps, speedFixPatch));

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
    return true;
}

bool patchResolution(Region const &text, Region const &data, sekiro::Resolution const &resolution)
{
    // Width, height, and scaling fix (allow widescreen) must all be applied or not at all
    uintptr_t resolutionAddress = data.findFirst(
        resolution.width < 1920
            ? sekiro::PATTERN_RESOLUTION_DEFAULT_720
            : sekiro::PATTERN_RESOLUTION_DEFAULT);

    uintptr_t scalingFixAddress = text.findFirst(sekiro::PATTERN_RESOLUTION_SCALING_FIX);
    std::vector<uint8_t> scalingFixPatch = hex::parse(sekiro::PATCH_RESOLUTION_SCALING_FIX_ENABLE);

    if (!resolutionAddress || !scalingFixAddress) {
        logWarning("Resolution or scaling fix not found");
        return false;
    }

    logInfo(std::format("Patching resolution to {}x{}", resolution.width, resolution.height));

    // Apply resolution
    data.write(resolutionAddress, &resolution, sizeof(resolution));

    // Apply resolution scaling fix
    text.write(scalingFixAddress, scalingFixPatch.data(), scalingFixPatch.size());

    logInfo("...Ok");
    return true;
}

bool patchCameraReset(Region const &text, Region const &data, bool enabled = false)
{
    // Default game behavior is enabled, so default patch is disable
    uintptr_t cameraResetAddress = text.findFirst(sekiro::PATTERN_CAMRESET_LOCKON);
    int cameraResetOffset = sekiro::PATTERN_CAMRESET_LOCKON_OFFSET;
    uint8_t cameraResetPatch = enabled ? sekiro::PATCH_CAMRESET_LOCKON_ENABLE : sekiro::PATCH_CAMRESET_LOCKON_DISABLE;

    if (!cameraResetAddress) {
        logWarning("Camera reset not found");
        return false;
    }

    logInfo(enabled ? "Disabling camera reset" : "Enabling camera reset");
    text.write(cameraResetAddress + cameraResetOffset, &cameraResetPatch, sizeof(cameraResetPatch));
    logInfo("...Ok");
    return true;
}

bool patchAutoloot(Region const &text, Region const &data, bool enabled = true)
{
    // Default game behavior is disabled, so default patch is enabled
    uintptr_t autolootAddress = text.findFirst(sekiro::PATTERN_AUTOLOOT);
    int autolootOffset = sekiro::PATTERN_AUTOLOOT_OFFSET;
    std::vector<uint8_t> autolootPatch = hex::parse(enabled ? sekiro::PATCH_AUTOLOOT_ENABLE : sekiro::PATCH_AUTOLOOT_DISABLE);

    if (!autolootAddress) {
        logWarning("Autoloot not found");
        return false;
    }

    logInfo(enabled ? "Enabling autoloot" : "Disabling autoloot");
    text.write(autolootAddress + autolootOffset, autolootPatch.data(), autolootPatch.size());
    logInfo("...Ok");
    return true;
}


// Main entry point

int main(int argc, char* args[])
{
    // Parse args and complain if invalid
    options opts = parseArgs(argc, args);

    // Debug output
    // log::setLogLevel(log::LogLevel::Debug); // comment to show
    logDebug(std::format("fps: {}, resolution: w{} h{}, cameraReset: {}, autoloot: {}, verbose: {}, timeout: {}, delay: {}",
        opts.fps, opts.resolution.width, opts.resolution.height, opts.cameraReset, opts.autoloot, opts.verbose, opts.timeout, opts.delay));

    // Set up logging
    log::setLogFormat(log::LogFormat::Compact);
    log::setLogLevel(opts.verbose ? log::LogLevel::Info : log::LogLevel::Warning);

    // Use IO if available, otherwise use PTRACE
    mem::setAccessMethod(mem::AccessMethod::IO);

    // Wait for process to start and get PID
    // for a Windows game, we are most interested in the .exe name
    std::string processName = sekiro::PROCESS_NAME;

    pid_t pid = proc::waitForProcess([&processName]() {
        return proc::getProcessIdByStatusName(processName);
    }, opts.timeout);

    if (pid <= 0) {
        logError(std::format("Failed to find process '{}'", processName));
        return 1;
    }

    // delay to allow process to start
    proc::wait(opts.delay);

    // Attach to process
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

    // Find the .text and .data sections
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

    // Apply patches

    // FPS
    if (opts.fps != -1) {
        if (!patchFps(text, data, opts.fps)) {
            logError("FPS patch failed");
            return 1;
        }
    }

    // Resolution
    if (opts.resolution.width != -1 && opts.resolution.height != -1) {
        if (!patchResolution(text, data, opts.resolution)) {
            logError("Resolution patch failed");
            return 1;
        }
    }


    // Disable Camera Reset
    if (opts.cameraReset) {
        if (!patchCameraReset(text, data)) {
            logError("Camera reset patch failed");
            return 1;
        }
    }

    // Enable Autoloot
    if (opts.autoloot) {
        if (!patchAutoloot(text, data)) {
            logError("Autoloot patch failed");
            return 1;
        }
    }

    // Done!
    logInfo("Done, enjoy!");
    proc::detach(pid);
    return 0;
}