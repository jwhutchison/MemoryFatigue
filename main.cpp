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
    std::string map;
    std::string showMaps;

    std::string section;
    long long address = -1;
    std::string pattern;
    size_t offset = 0;
    int read = -1;
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
        // Process options
        TCLAP::ValueArg<int> pidArg("p", "pid", "Use a specific process PID", false, -1, "int", cmd);
        TCLAP::ValueArg<std::string> cmdlineArg("c", "cmdline", "Find a process by cmdline (whole or part)", false, "", "string", cmd);
        TCLAP::ValueArg<std::string> statusNameArg("s", "status", "Find a process by status name", false, "", "string", cmd);
        TCLAP::ValueArg<std::string> mapArg("m", "map", "Find the process map by name (whole or part, if different from status name)", false, "", "string", cmd);
        TCLAP::ValueArg<std::string> showMapsArg("", "show-maps", "List all process maps; filter by name, 'file', 'all'", false, "", "string", cmd);

        // Location options
        TCLAP::ValueArg<std::string> sectionArg("", "section", "Section name to search for pattern (default '.text')", false, ".text", "string", cmd);
        TCLAP::ValueArg<long long> addressArg("", "address", "Absolute address in section (skip pattern search)", false, -1, "int", cmd);
        TCLAP::ValueArg<std::string> patternArg("", "pattern", "Pattern to search for in section", false, "", "string", cmd);
        TCLAP::ValueArg<size_t> offsetArg("", "offset", "Offset from pattern to patch (default 0)", false, 0, "int", cmd);

        // Actions
        TCLAP::ValueArg<int> readArg("", "read", "Read and display a number of bytes at offset", false, -1, "int", cmd);
        TCLAP::ValueArg<std::string> patchArg("", "patch", "Patch to apply at offset", false, "", "string", cmd);

        // Flags
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
        opts.map = mapArg.getValue();

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

        // If show-maps is specified, it's allowed to be empty (default 'file')
        opts.showMaps = string::toLower(showMapsArg.getValue());

        // Locate and Action opts
        opts.section = string::toLower(sectionArg.getValue());
        opts.address = addressArg.getValue();
        opts.pattern = patternArg.getValue();
        opts.offset = offsetArg.getValue();
        opts.patch = patchArg.getValue();
        opts.read = readArg.getValue();

        // Only one of pattern or address can be specified
        if (!opts.pattern.empty() && opts.address >= 0) {
            TCLAP::ArgException err("Pattern and address cannot both be used", "pattern/address");
            out.failure(cmd, err);
        }

        // Only one of read or patch can be specified
        if (opts.read >= 0 && !opts.patch.empty()) {
            TCLAP::ArgException err("Read and patch cannot both be used", "read/patch");
            out.failure(cmd, err);
        }

        // If pattern is specified, section must be specified
        if (!opts.pattern.empty() && opts.section.empty()) {
            TCLAP::ArgException err("Section must be specified when using pattern", "section");
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

        // Flags
        opts.dryRun = dryRunArg.getValue();
        opts.interactive = interactiveArg.getValue();
        opts.verbose = verboseArg.getValue();
        opts.ptrace = ptraceArg.getValue();
        opts.timeout = timeoutArg.getValue();
        opts.delay = delayArg.getValue();

        // Some options require verbose to make any sense
        if (opts.interactive || opts.dryRun || opts.read >= 0 || opts.patch.empty()) {
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

// Confirm before continuing in interactive mode
void confirm()
{
    std::cout << "Continue? [y/N] ";
    std::string yn;
    std::getline(std::cin, yn);

    // Any key other than 'y' or 'Y' will exit
    if (yn.empty() || (yn[0] != 'y' && yn[0] != 'Y')) {
        std::cout << "Exiting..." << std::endl;
        exit(0);
    }
}

// Main entry point

int main(int argc, char* args[])
{
    /****************************************************
     * Parse and validate command line arguments
     ****************************************************/

    options opts = parseArgs(argc, args);

    // Set up logging
    log::setLogFormat(log::LogFormat::NoLabel);
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

    /****************************************************
     * Find and attach to process
     ****************************************************/

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
        processName = proc::getStatusName(pid);
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
    logInfo(std::format("Cmdline:     {}\n", proc::getCmdline(pid)));

    // If show maps, this is as far as we go
    if (!opts.showMaps.empty()) {
        if (opts.showMaps == "all") {
            logInfo("Dumping all process maps, good luck!");
            confirm();
        } else if (opts.showMaps == "file") {
            logInfo("Dumping process maps that are real files");
        } else {
            logInfo(std::format("Dumping process maps that contain '{}'", opts.showMaps));
        }

        for (auto &map : proc::getMaps(pid)) {
            if (
                opts.showMaps == "all"
                || (opts.showMaps == "file" && map.isFile())
                || map.name.contains(opts.showMaps)
            ) {
                std::cout
                    << Color::Dim << std::format("{:#x}", map.start) << Color::Reset << "-"
                    << Color::Dim << std::format("{:#x}", map.end) << Color::Reset << " ";

                if (map.isRead()) std::cout << Color::Green << "r" << Color::Reset;
                else std::cout << Color::Dim << "-" << Color::Reset;

                if (map.isWrite()) std::cout << Color::Yellow << "w" << Color::Reset;
                else std::cout << Color::Dim << "-" << Color::Reset;

                if (map.isExec()) std::cout << Color::Cyan << "x" << Color::Reset;
                else std::cout << Color::Dim << "-" << Color::Reset;

                if (map.isPrivate()) std::cout << Color::Red << "p" << Color::Reset;
                else std::cout << Color::Dim << "s" << Color::Reset;

                std::cout << " "
                    << Color::Blue << std::format("{:#x}", map.offset) << Color::Reset << " "
                    << map.name << std::endl;
            }
        }

        return 0;
    } // end show maps

    // If no address or pattern, then we're done
    if (opts.address < 0 && opts.pattern.empty()) {
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

    /****************************************************
     * Find the process map and working region
     ****************************************************/

    proc::Map map = opts.map.empty()
        ? proc::findMapEndsWith(pid, processName)
        : proc::findMap(pid, opts.map);

    if (!map.isValid()) {
        logError("Failed to find map");
        return 1;
    }

    Region section;

    // Allow reading the process map directly (useful for checking headers)
    if (
        (opts.section == "map" || opts.section == "header")
        && opts.address >= 0 && opts.read >= 0
    ) {
        // This is a special case, and will ignore the pattern and offset and only allows read
        section = map;

    } else if (pe::isValidPE(map)) {
        // Otherwise, check if process is PE and read the section
        pe::PeMap peMap(map);

        if (!peMap.isValid()) {
            logError("Failed to read PE headers");
            return 1;
        }

        section = peMap.getSection(opts.section);

    } else if (elf::isValidElf(map)) {
        // Otherwise, check if process is ELF (section is ignored)
        elf::ElfMap elfMap(map);

        if (!elfMap.isValid()) {
            logError("Failed to read ELF headers");
            return 1;
        }

        // ! IMPORTANT: ELF support is experimental, but will likely work for read operations
        // ! Because of this, we are going to print a huge warning to the user and force interactive mode
        if (!opts.patch.empty()) {
            logWarning(
                "Patching ELF memory regions is experimental and may cause crashes or data corruption!"
                "\nProceed only if you are sure (switching to --interactive to confirm)"
            );
            opts.interactive = true;
        }

        section = elfMap.getLoadedRegion();

    } else {
        logError("Failed to read PE or ELF headers");
        return 1;
    }

    if (!section.isValid()) {
        logError(std::format("Failed to find section '{}'", opts.section));
        return 1;
    }

    /****************************************************
     * Create the Patch object and perform the action
     ****************************************************/

    if (opts.interactive) confirm();

    if (opts.address >= 0 && opts.offset != 0) {
        logWarning("Ignoring offset for specified address");
    }

    Patch patch;

    // Get the address from the pattern or use the specified address
    if (opts.address >= 0) {
        patch = Patch(section, opts.address, opts.patch);
    } else {
        patch = Patch(section, opts.pattern, opts.offset, opts.patch);

        if (!patch.isValid()) {
            logError("Pattern not found");
            return 1;
        }
    }

    if (opts.read >= 0) {
        // if read, read and display the bytes
        std::vector<uint8_t> data(opts.read);
        if (patch.region().read(patch.address(), data.data(), data.size())) {
            logInfo(std::format(
                "Read {} bytes in region {}\n{}",
                data.size(), patch.region().toString(),
                hex::dump(data.data(), data.size(), patch.address())
            ));
        } else {
            logError(std::format("Failed to read {} bytes at {:#x} in {}", opts.read, patch.address(), patch.region().toString()));
            return 1;
        }

    } else if (!opts.patch.empty()) {
        // if patch, display and apply the patch
        logInfo(patch.dump());

        // if dry run, we're done
        if (opts.dryRun) {
            logInfo("Dry run, exiting");
            return 0;
        }

        // Confirm before continuing in interactive mode
        if (opts.interactive) confirm();

        // Apply the patch
        if (patch.apply()) {
            logInfo(std::format("Patch applied\n{}", patch.dumpPatch()));
        } else {
            logError("Failed to apply patch");
            return 1;
        }

    } else {
        // Nothing to do, just print some stuff
        if (opts.address >= 0) {
            logInfo(std::format("Address {:#x} in region {}", opts.address, patch.region().toString()));
        } else {
            logInfo(std::format("Pattern search in region {}\n{}", patch.region().toString(), patch.dumpPattern()));
        }
    }

    return 0;
}