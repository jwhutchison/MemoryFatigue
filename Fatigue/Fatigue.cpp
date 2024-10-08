#include "KittyUtils.hpp"
#include "Fatigue.hpp"
#include "utils.hpp"

namespace fatigue {

    std::string getProcessStatusName(pid_t pid)
    {
        if (pid <= 0)
            return "";

        char status[256] = {0};
        snprintf(status, sizeof(status), "/proc/%d/status", pid);

        errno = 0;
        FILE *fp = fopen(status, "r");
        if (!fp) {
            KITTY_LOGE("Couldn't open status file %s, error=%s", status, strerror(errno));
            return "";
        }

        std::string name = "";

        char line[128] = {0};
        while (fgets(line, sizeof(line), fp)) {
            if (string::startsWith(line, "Name:")) {
                name = line + 5;    // skip "Name:"
                string::trim(name); // remove leading/trailing spaces
                break;
            }
        }

        fclose(fp);
        return name;
    }

    pid_t getProcessIDByComparator(const std::string &processName, bool (*comparator)(const std::string &, const int))
    {
        if (processName.empty())
            return 0;

        pid_t pid = 0;

        errno = 0;
        DIR *dir = opendir("/proc/");
        if (!dir)
        {
            KITTY_LOGE("Couldn't open /proc/, error=%s", strerror(errno));
            return pid;
        }

        dirent *entry = nullptr;
        while ((entry = readdir(dir)) != nullptr)
        {
            int entryPid = atoi(entry->d_name);
            if (entryPid > 0)
            {
                if (comparator(processName, entryPid))
                {
                    pid = entryPid;
                    break;
                }
            }
        }
        closedir(dir);
        return pid;
    }

    pid_t getProcessIDByStatusName(const std::string &processName)
    {
        return getProcessIDByComparator(processName, [](const std::string &processName, const int checkPid) {
            return processName == getProcessStatusName(checkPid);
        });
    }

    pid_t getProcessIDByCmdlineEndsWith(const std::string &processName)
    {
        return getProcessIDByComparator(processName, [](const std::string &processName, const int checkPid) {
            return string::endsWith(getProcessName(checkPid), processName);
        });
    }

    pid_t getProcessIDByCmdlineContains(const std::string &processName)
    {
        return getProcessIDByComparator(processName, [](const std::string &processName, const int checkPid) {
            return string::contains(getProcessName(checkPid), processName);
        });
    }
}
