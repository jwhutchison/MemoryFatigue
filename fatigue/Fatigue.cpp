#include "KittyUtils.hpp"
#include "Fatigue.hpp"
#include "PE.hpp"
#include "utils.hpp"

namespace fatigue {
    // Pre-init helper functions

    std::string getStatusName(pid_t pid)
    {
        if (pid <= 0) return "";

        char status[256] = {0};
        snprintf(status, sizeof(status), "/proc/%d/status", pid);

        errno = 0;
        FILE* fp = fopen(status, "r");
        if (!fp) {
            KITTY_LOGE("Couldn't open status file %s, error=%s", status,
                       strerror(errno));
            return "";
        }

        char line[128] = {0};
        while (fgets(line, sizeof(line), fp)) {
            if (string::startsWith(line, "Name:")) {
                std::string name = line + 5; // skip "Name:"
                return string::trim(name); // remove leading/trailing spaces
            }
        }

        fclose(fp);
        return "";
    }

    pid_t getProcessID(const std::string& processName,
                       bool (*comparator)(const std::string&, const int))
    {
        if (processName.empty()) return 0;

        pid_t pid = 0;

        errno = 0;
        DIR* dir = opendir("/proc/");
        if (!dir) {
            KITTY_LOGE("Couldn't open /proc/, error=%s", strerror(errno));
            return pid;
        }

        dirent* entry = nullptr;
        while ((entry = readdir(dir)) != nullptr) {
            int entryPid = atoi(entry->d_name);
            if (entryPid > 0) {
                if (comparator(processName, entryPid)) {
                    pid = entryPid;
                    break;
                }
            }

            delete entry;
        }

        closedir(dir);
        return pid;
    }

    pid_t getProcessIDByStatusName(const std::string& processName)
    {
        return getProcessID(processName, [](const std::string& processName, const int checkPid) {
            return processName == getStatusName(checkPid);
        });
    }

    pid_t getProcessIDByCmdlineEndsWith(const std::string& processName)
    {
        return getProcessID(processName, [](const std::string& processName, const int checkPid) {
            return string::endsWith(getProcessName(checkPid), processName);
        });
    }

    pid_t getProcessIDByCmdlineContains(const std::string& processName)
    {
        return getProcessID(processName, [](const std::string& processName, const int checkPid) {
            return string::contains(getProcessName(checkPid), processName);
        });
    }

    // Fatigue factories

    Fatigue* Fatigue::attach(pid_t pid, EKittyMemOP eMemOp, bool initMemPatch)
    {
        auto* instance = new Fatigue(pid, eMemOp, initMemPatch);
        if (!instance->isReady()) {
            delete instance;
            return nullptr;
        }
        return instance;
    }

    Fatigue* Fatigue::attach(const std::string &processName, EKittyMemOP eMemOp, bool initMemPatch)
    {
        return Fatigue::attach(fatigue::getProcessID(processName), eMemOp, initMemPatch);
    }

    Fatigue* Fatigue::attach(
        const std::string &processName,
        std::function<pid_t(const std::string&)> getPidFn,
        EKittyMemOP eMemOp,
        bool initMemPatch)
    {
        return Fatigue::attach(getPidFn(processName), eMemOp, initMemPatch);
    }

    // Fatigue class methods

    bool Fatigue::initialize(pid_t pid, EKittyMemOP eMemOp, bool initMemPatch)
    {
        // Initialize KittyMemoryMgr
        if (!KittyMemoryMgr::initialize(pid, eMemOp, initMemPatch)) return false;

        // Add Fatigue extras
        m_statusName = fatigue::getStatusName(pid);
        peScanner = pe::PeScannerMgr(_pMemOp.get());

        return true;
    }

    bool Fatigue::isValidPe(uintptr_t base) const
    {
        uint16_t magic = 0;
        readMem(base, &magic, sizeof(uint16_t));
        return magic == pe::DOS_MAGIC; // MZ
    }

    pe::SectionScanner* Fatigue::getPeSection(const std::string &sectionName)
    {
        return peScanner.isValid() ? peScanner.getSection(sectionName) : nullptr;
    }
} // namespace fatigue
