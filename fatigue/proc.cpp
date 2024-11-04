#include <chrono>
#include <filesystem>
#include <fstream>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <thread>
#include "fatigue.hpp"
#include "log.hpp"

namespace fatigue::proc {

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

    // Process ID

    pid_t getProcessID(const std::string& processName, std::function<bool(pid_t)> filter)
    {
        if (processName.empty()) return 0;

        pid_t pid = 0;

        const std::filesystem::path proc{"/proc/"};
        for (auto const& entry : std::filesystem::directory_iterator{proc}) {
            if (entry.is_directory()) {
                pid_t entryPid = atoi(entry.path().filename().c_str());
                if (entryPid > 0) {
                    if (filter(entryPid)) {
                        pid = entryPid;
                        break;
                    }
                }
            }

        }

        return pid;
    }

    pid_t getProcessId(const std::string& cmdline) {
        return getProcessID(cmdline, [&](const int checkPid) {
            return getCmdline(checkPid) == cmdline;
        });
    }

    pid_t getProcessIdByStatusName(const std::string& processName)
    {
        return getProcessID(processName, [&](const int checkPid) {
            return getStatusName(checkPid) == processName;
        });
    }

    pid_t getProcessIdByCmdlineEndsWith(const std::string& processName)
    {
        return getProcessID(processName, [&](const int checkPid) {
            return getCmdline(checkPid).ends_with(processName);
        });
    }

    pid_t getProcessIdByCmdlineContains(const std::string& processName)
    {
        return getProcessID(processName, [&](const int checkPid) {
            return getCmdline(checkPid).contains(processName);
        });
    }

    pid_t waitForProcess(std::function<pid_t()> getter, u_int timeout, u_int interval)
    {
        if (!getter) return 0;
        if (timeout <= 0) return getter();

        pid_t pid = 0;
        while (pid <= 0) {
            pid = getter();
            if (pid <= 0) {
                if (timeout-- <= 0) {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::seconds(interval));
            }
        }
        return pid;
    }

    // Maps

    std::vector<Map> getMaps(pid_t pid, std::function<bool(Map&)> filter)
    {
        std::vector<Map> maps{};
        if (pid <= 0) return maps;

        std::filesystem::path path = std::format("/proc/{}/maps", pid);
        std::ifstream file(path);
        if (!file.is_open()) {
            logError(std::format("Error opening {}", path.c_str()));
            return maps;
        }

        try {
            std::string line;
            while (std::getline(file, line)) {
                Map map{};
                map.pid = pid;

                // Throwaway variable for the dash
                char dash;
                // (format) start-end perms offset dev inode pathname
                std::istringstream iss{line};
                iss >> std::hex >> map.start >> dash >> map.end
                    >> map.perms >> map.offset >> map.dev
                    >> std::dec >> map.inode >> map.name;

                if (!filter || filter(map)) {
                    maps.push_back(map);
                }
            }
        } catch (const std::exception& e) {
            logError(std::format("Error reading {}: {}", path.c_str(), e.what()));
        }

        return maps;
    }

    std::vector<Map> getValidMaps(pid_t pid)
    {
        return getMaps(pid, [](Map& map) {
            return map.isValid() && !map.isAnonymous();
        });
    }

    std::vector<Map> getMaps(pid_t pid, const std::string& name)
    {
        return getMaps(pid, [&name](Map& map) {
            return map.isValid() && map.name.contains(name);
        });
    }

    std::vector<Map> getMapsEndsWith(pid_t pid, const std::string& name)
    {
        return getMaps(pid, [&name](Map& map) {
            return map.isValid() && map.name.ends_with(name);
        });
    }

    Map findMap(pid_t pid, const std::string& name)
    {
        std::vector<Map> maps = getMaps(pid, name);
        if (maps.empty()) return Map{};
        return maps.front();
    }

    Map findMapEndsWith(pid_t pid, const std::string& name)
    {
        std::vector<Map> maps = getMapsEndsWith(pid, name);
        if (maps.empty()) return Map{};
        return maps.front();
    }

    // Attach and detach

    bool attach(pid_t pid)
    {
        if (pid <= 0) return -1;

        // attach to the process, this will initiate a stop
        if(ptrace(PTRACE_ATTACH, pid, 0, 0) != 0) {
            logError(std::format("Failed to attach to process {}", pid));
            return false;
        }

        // wait for the process to stop
        int status = 0;
        do {
            waitpid(pid, &status, 0);

            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                logError(std::format("Process {} exited while waiting for stop", pid));
                return false;
            }
        } while (!WIFSTOPPED(status));

        return true;
    }

    bool detach(pid_t pid)
    {
        if (pid <= 0) return -1;

        // detach from the process, this will resume the process
        return ptrace(PTRACE_DETACH, pid, 0, 0) == 0;
    }

} // namespace fatigue
