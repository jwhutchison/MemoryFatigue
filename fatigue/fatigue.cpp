#include <cstring>
#include <dirent.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <string>
#include "fatigue.hpp"
#include "log.hpp"

namespace fatigue {

    std::map<std::string, std::string> getStatus(pid_t pid)
    {
        std::map<std::string, std::string> status{};
        if (pid <= 0) return status;

        std::filesystem::path path = std::format("/proc/{}/status", pid);
        std::ifstream file(path);
        if (!file.is_open()) {
            logError(std::format("Error opening {}", path.c_str()));
            return status;
        }

        try {
            std::string line;
            while (std::getline(file, line)) {
                std::istringstream iss{line};

                // Get key, remove trailing : and whitespace
                std::string key;
                iss >> key;

                key = string::trim(key);
                if (key.ends_with(':')) key = key.substr(0, key.size() - 1);

                // Get value
                std::string value{""};
                std::string token;
                while (iss >> token) {
                    value += token + " ";
                }

                // Trim value
                status[key] = string::trim(value);
            }
        } catch (const std::exception& e) {
            logError(std::format("Error reading {}: {}", path.c_str(), e.what()));
            return status;
        }

        return status;
    }

    std::string getStatusName(pid_t pid)
    {
        if (pid <= 0) return "";

        std::map<std::string, std::string> status = getStatus(pid);
        if (status.find("Name") != status.end()) {
            return status["Name"];
        }

        return "";
    }

    std::string getCmdline(pid_t pid)
    {
        if (pid <= 0) return "";

        std::filesystem::path path = std::format("/proc/{}/cmdline", pid);
        std::ifstream file(path);
        if (!file.is_open()) {
            logError(std::format("Error opening {}", path.c_str()));
            return "";
        }

        try {
            std::string buffer;
            file >> buffer;
            return string::trim(buffer);
        } catch (const std::exception& e) {
            logError(std::format("Error reading {}: {}", path.c_str(), e.what()));
            return "";
        }
    }

    pid_t getProcessID(const std::string& processName,
                       bool (*comparator)(const std::string&, const pid_t))
    {
        if (processName.empty()) return 0;

        pid_t pid = 0;

        const std::filesystem::path proc{"/proc/"};
        for (auto const& entry : std::filesystem::directory_iterator{proc}) {
            if (entry.is_directory()) {
                pid_t entryPid = atoi(entry.path().filename().c_str());
                if (entryPid > 0) {
                    if (comparator(processName, entryPid)) {
                        pid = entryPid;
                        break;
                    }
                }
            }

        }

        return pid;
    }

    pid_t getProcessId(const std::string& cmdline) {
        return getProcessID(cmdline, [](const std::string& processName, const int checkPid) {
            return getCmdline(checkPid) == processName;
        });
    }

    pid_t getProcessIdByStatusName(const std::string& processName)
    {
        return getProcessID(processName, [](const std::string& processName, const int checkPid) {
            return getStatusName(checkPid) == processName;
        });
    }

    pid_t getProcessIdByCmdlineEndsWith(const std::string& processName)
    {
        return getProcessID(processName, [](const std::string& processName, const int checkPid) {
            return getCmdline(checkPid).ends_with(processName);
        });
    }

    pid_t getProcessIdByCmdlineContains(const std::string& processName)
    {
        return getProcessID(processName, [](const std::string& processName, const int checkPid) {
            return getCmdline(checkPid).contains(processName);
        });
    }
} // namespace fatigue
