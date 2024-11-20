#include <format>
#include <string>
#include <tclap/CmdLine.h>
#include "fatigue.hpp"

using namespace fatigue;

// Define and parse command line arguments

struct options {
    int pid = -1;
    std::string cmdline;
    std::string statusName;

    std::string section;
    std::string pattern;
    size_t offset = 0;
    std::string patch;

    bool dryRun = false;
    bool interactive = false;
    bool verbose = false;
    bool ptrace = false;
    int timeout = -1;
    int delay = -1;
};

options parseArgs(int argc, char* args[]) {
    try {
        // Define the command line parser
        TCLAP::CmdLine cmd("Memory Fatigue PE process patcher", ' ', "1.0");

        TCLAP::ValueArg<int> pidArg("p", "pid", "Use a specific process PID", false, -1, "int", cmd);
        TCLAP::ValueArg<std::string> cmdlineArg("c", "cmdline", "Find a process by cmdline (whole or part)", false, "", "string", cmd);
        TCLAP::ValueArg<std::string> statusNameArg("s", "status", "Find a process by status name", false, "", "string", cmd);

        TCLAP::ValueArg<std::string> sectionArg("", "section", "Section name to search for pattern (default '.text')", false, ".text", "string", cmd);
        TCLAP::ValueArg<std::string> patternArg("", "pattern", "Pattern to search for in section", false, "", "string", cmd);
        TCLAP::ValueArg<size_t> offsetArg("", "offset", "Offset from pattern to patch (default 0)", false, 0, "int", cmd);
        TCLAP::ValueArg<std::string> patchArg("", "patch", "Patch to apply at offset", false, "", "string", cmd);

        TCLAP::SwitchArg dryRunArg("d", "dry-run", "Dry run, don't apply patches", cmd);
        TCLAP::SwitchArg interactiveArg("i", "interactive", "Interactive mode, prompt before applying patches", cmd);
        TCLAP::SwitchArg verboseArg("v", "verbose", "Verbose output", cmd);
        TCLAP::SwitchArg ptraceArg("P", "ptrace", "Use PTRACE for memory access", cmd);
        TCLAP::ValueArg<int> timeoutArg("T", "timeout", "Seconds to wait for process to start", false, 30, "int", cmd);
        TCLAP::ValueArg<int> delayArg("D", "delay", "Milliseconds to wait after process starts (increase if errors on start)", false, 1000, "int", cmd);

        cmd.parse(argc, args);

        // Borrow the TCLAP output handler so we can format our consistency errors
        TCLAP::StdOutput out;

        // Populate and validate arguments
        options opts;

        // Process opts
        opts.pid = pidArg.getValue();
        opts.cmdline = cmdlineArg.getValue();
        opts.statusName = statusNameArg.getValue();

        // At least one of pid, status, or cmdline must be specified
        if (opts.pid <=0 && opts.statusName.empty() && opts.cmdline.empty()) {
            TCLAP::ArgException err("At least one of PID, status, or cmdline must be specified", "pid/status/cmdline");
            out.failure(cmd, err);
        }
        // but only one
        if (
            (opts.pid > 0 && (!opts.cmdline.empty() || !opts.statusName.empty())) ||
            (!opts.cmdline.empty() && !opts.statusName.empty())
        ) {
            TCLAP::ArgException err("Only one of PID, status, or cmdline can be specified", "pid/status/cmdline");
            out.failure(cmd, err);
        }

        // Patch opts
        opts.section = sectionArg.getValue();
        opts.pattern = patternArg.getValue();
        opts.offset = offsetArg.getValue();
        opts.patch = patchArg.getValue();

        // If pattern is specified, section must also be specified
        if (!opts.pattern.empty() && opts.section.empty()) {
            TCLAP::ArgException err("Section must be specified if pattern is specified", "section");
            out.failure(cmd, err);
        }

        // If specified, pattern must be a hex string (wildcards allowed)
        if (!opts.pattern.empty() && !hex::isValid(opts.pattern, false)) {
            TCLAP::ArgException err("Pattern must be a valid hex string (wildcards allowed)", "pattern");
            out.failure(cmd, err);
        }

        // If specified, patch must be a hex string (no wildcards)
        if (!opts.patch.empty() && !hex::isValid(opts.patch, true)) {
            TCLAP::ArgException err("Patch must be a valid hex string", "patch");
            out.failure(cmd, err);
        }

        // opts
        opts.dryRun = dryRunArg.getValue();
        opts.interactive = interactiveArg.getValue();
        opts.verbose = verboseArg.getValue();
        opts.ptrace = ptraceArg.getValue();
        opts.timeout = timeoutArg.getValue();
        opts.delay = delayArg.getValue();

        // Some options require verbose to make any sense
        if (opts.interactive || opts.dryRun || opts.patch.empty()) {
            opts.verbose = true;
        }

        return opts;
    } catch (const TCLAP::ArgException &e) {
        std::cerr << e.error() << " for arg " << e.argId() << std::endl;
        exit(1);
    } catch (const TCLAP::ExitException &e) {
        exit(e.getExitStatus());
    }
}

// Main entry point

int main(int argc, char* args[])
{
    // Parse args and complain if invalid
    options opts = parseArgs(argc, args);

    // Set up logging
    log::setLogFormat(log::LogFormat::Tiny);
    log::setLogLevel(opts.verbose ? log::LogLevel::Info : log::LogLevel::Warning);

    // Debug options
    // log::setLogLevel(log::LogLevel::Debug); // uncomment to show debug
    logDebug(std::format(
        "pid: {}, status: '{}', cmdline: '{}'\n"
        "section: '{}', pattern: '{}', offset: {}, patch: '{}'\n"
        "dryRun: {}, interactive: {}, verbose: {}\n"
        "ptrace: {}, timeout: {}, delay: {}",
        opts.pid, opts.statusName, opts.cmdline,
        opts.section, opts.pattern, opts.offset, opts.patch,
        opts.dryRun, opts.interactive, opts.verbose,
        opts.ptrace, opts.timeout, opts.delay
    ));

    // Use IO if available, otherwise use PTRACE
    if (opts.ptrace) {
        logInfo("Using PTRACE for memory access");
        mem::setAccessMethod(mem::AccessMethod::PTRACE);
    } else {
        mem::setAccessMethod(mem::AccessMethod::IO);
    }

    // Find and attach to process
    pid_t pid = proc::waitForProcess([&opts]() {
        if (opts.pid > 0) {
            return opts.pid;
        } else if (!opts.statusName.empty()) {
            return proc::getProcessIdByStatusName(opts.statusName);
        } else if (!opts.cmdline.empty()) {
            return proc::getProcessIdByCmdlineContains(opts.cmdline);
        }
        return 0;
    }, opts.timeout);

    std::string processName;

    if (opts.pid > 0) {
        processName = proc::getCmdline(pid);
    } else if (!opts.statusName.empty()) {
        processName = opts.statusName;
    } else if (!opts.cmdline.empty()) {
        processName = opts.cmdline;
    }

    if (pid <= 0) {
        logError(std::format("Failed to find process '{}'", processName));
        return 1;
    }

    logInfo(std::format("Process ID:  {}", pid));
    logInfo(std::format("Status Name: {}", proc::getStatusName(pid)));
    logInfo(std::format("Cmdline:     {}", proc::getCmdline(pid)));

    // If no pattern, then we're done
    if (opts.pattern.empty()) {
        logInfo("No pattern specified, exiting");
        return 0;
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

    if (opts.interactive) {
        logInfo("Press any key to continue or Ctrl+C to exit...");
        std::cin.get();
    }

    // Find the section
    Region section = peMap.getSection(opts.section);

    if (!section.isValid()) {
        logError(std::format("Failed to find section '{}'", opts.section));
        return 1;
    }

    // Search for the pattern
    Patch patch(section, opts.pattern, opts.offset, opts.patch);

    if (!patch.isValid()) {
        logError("Pattern not found");
        return 1;
    }

    // If no patch, print found pattern and exit
    if (opts.patch.empty()) {
        logInfo(std::format("Pattern search in region {}\n{}", patch.region().toString(), patch.dumpPattern()));
        return 0;
    }

    logInfo(patch.dump());

    // if dry run, we're done
    if (opts.dryRun) {
        logInfo("Dry run, exiting");
        return 0;
    }

    if (opts.interactive) {
        logInfo("Press any key to apply the patch or Ctrl+C to exit...");
        std::cin.get();
    }

    // Apply the patch
    if (patch.apply()) {
        logInfo(std::format("Patch applied\n{}", patch.dumpPatch()));
    } else {
        logError("Failed to apply patch");
    }

    return 0;
}