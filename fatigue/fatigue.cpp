#include <cstring>
#include <dirent.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include "fatigue.hpp"
#include "log.hpp"

namespace fatigue {

    // std::string getStatusName(pid_t pid)
    // {
    //     if (pid <= 0) return "";

    //     char status[256] = {0};
    //     snprintf(status, sizeof(status), "/proc/%d/status", pid);

    //     errno = 0;
    //     FILE* fp = fopen(status, "r");
    //     if (!fp) {
    //         logError(std::format("Couldn't open status file %s, error=%s", status,
    //                    strerror(errno)));
    //         return "";
    //     }

    //     char line[128] = {0};
    //     while (fgets(line, sizeof(line), fp)) {
    //         if (string::startsWith(line, "Name:")) {
    //             std::string name = line + 5; // skip "Name:"
    //             return string::trim(name); // remove leading/trailing spaces
    //         }
    //     }

    //     fclose(fp);
    //     return "";
    // }

    std::string getCmdline(pid_t pid)
    {
        if (pid <= 0)
            return "";

        std::filesystem::path path = std::format("/proc/{}/cmdline", pid);
        std::ifstream file(path);

        if (!file.is_open()) {
            // logError(std::format("Couldn't open cmdline file {}, error={}", path, file.rdstate()));
            return "";
        }

        std::string cmdline;
        std::getline(file, cmdline);
        return cmdline;
    }

    // pid_t getProcessID(const std::string& processName,
    //                    bool (*comparator)(const std::string&, const int))
    // {
    //     if (processName.empty()) return 0;

    //     pid_t pid = 0;

    //     errno = 0;
    //     DIR* dir = opendir("/proc/");
    //     if (!dir) {
    //         logError(std::format("Couldn't open /proc/, error=%s", strerror(errno)));
    //         return pid;
    //     }

    //     dirent* entry = nullptr;
    //     while ((entry = readdir(dir)) != nullptr) {
    //         int entryPid = atoi(entry->d_name);
    //         if (entryPid > 0) {
    //             if (comparator(processName, entryPid)) {
    //                 pid = entryPid;
    //                 break;
    //             }
    //         }

    //         delete entry;
    //     }

    //     closedir(dir);
    //     return pid;
    // }

    // pid_t getProcessIDByStatusName(const std::string& processName)
    // {
    //     return getProcessID(processName, [](const std::string& processName, const int checkPid) {
    //         return processName == getStatusName(checkPid);
    //     });
    // }

    // pid_t getProcessIDByCmdlineEndsWith(const std::string& processName)
    // {
    //     return getProcessID(processName, [](const std::string& processName, const int checkPid) {
    //         return string::endsWith(getProcessName(checkPid), processName);
    //     });
    // }

    // pid_t getProcessIDByCmdlineContains(const std::string& processName)
    // {
    //     return getProcessID(processName, [](const std::string& processName, const int checkPid) {
    //         return string::contains(getProcessName(checkPid), processName);
    //     });
    // }
} // namespace fatigue
