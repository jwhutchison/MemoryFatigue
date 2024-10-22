#include <cstring>
#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "fatigue.hpp"
#include "log.hpp"
#include <sys/ptrace.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/uio.h>

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
                // (format) startAddress-endAddress perms offset dev inode pathname
                std::istringstream iss{line};
                iss >> map.startAddress >> dash >> map.endAddress
                    >> map.perms >> map.offset >> map.dev >> map.inode >> map.pathname;

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
            return map.isValid() && map.hasName();
        });
    }

    std::vector<Map> getMaps(pid_t pid, const std::string& name)
    {
        return getMaps(pid, [&name](Map& map) {
            return map.isValid() && map.pathname.contains(name);
        });
    }

    std::vector<Map> getMapsEndsWith(pid_t pid, const std::string& name)
    {
        return getMaps(pid, [&name](Map& map) {
            return map.isValid() && map.pathname.ends_with(name);
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

    // Memory

    bool readMem(pid_t pid, uintptr_t address, void* buffer, size_t size)
    {
        if (pid <= 0 || address <= 0 || size <= 0) return {};

        std::filesystem::path path = std::format("/proc/{}/mem", pid);
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            logError(std::format("Error opening /proc/{}/mem", pid));
            return {};
        }

        file.seekg(address);
        file.read(static_cast<char*>(buffer), size);

        if (file.fail()) {
            logError(std::format("Error reading /proc/{}/mem", pid));
            return {};
        }

        return buffer;
    }

    ssize_t readMem2(pid_t pid, uintptr_t address, void* buffer, size_t size)
    {
        if (pid <= 0 || address <= 0 || size <= 0) return {};

        std::filesystem::path path = std::format("/proc/{}/mem", pid);
        auto fd = open(path.c_str(), O_RDWR);

        ptrace(PTRACE_ATTACH, pid, 0, 0);
        waitpid(pid, NULL, 0);

        ssize_t bytesRead = pread(fd, &buffer, size, address);
        // pwrite(fd, &value, sizeof(value), addr);

        ptrace(PTRACE_DETACH, pid, 0, 0);

        close(fd);

        return bytesRead;
    }

    ssize_t readMem3(pid_t pid, uintptr_t address, void* buffer, size_t size)
    {
        if (pid <= 0 || address <= 0 || size <= 0) return {};

        struct iovec local = { .iov_base = buffer, .iov_len = size };
        struct iovec remote = { .iov_base = reinterpret_cast<void*>(address), .iov_len = size };

        ssize_t bytesRead = process_vm_readv(pid, &local, 1, &remote, 1, 0);

        return bytesRead;
    }
} // namespace fatigue
