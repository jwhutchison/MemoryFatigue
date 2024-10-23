#include "pe.hpp"
#include "mem.hpp"

using namespace fatigue::mem::sys;

namespace fatigue::pe {
    bool isValidPE(pid_t pid, uintptr_t address) {
        return false;
    }

    bool isValidPE(proc::Map& map) {
        if (!map.isValid() || map.isAnonymous()) return false;
        return isValidPE(map.pid, map.start);
    }
} // namespace fatigue::pe
