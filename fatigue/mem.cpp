#include <fcntl.h>
#include <filesystem>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include "mem.hpp"

namespace fatigue::mem {
    namespace io {
        void attach(pid_t pid)
        {
            if (pid <= 0) return;
            ptrace(PTRACE_ATTACH, pid, 0, 0);
            waitpid(pid, NULL, 0);
        }

        void detach(pid_t pid)
        {
            if (pid <= 0) return;
            ptrace(PTRACE_DETACH, pid, 0, 0);
        }

        ssize_t read(pid_t pid, uintptr_t address, void* buffer, size_t size, bool usePtrace)
        {
            if (pid <= 0 || address <= 0 || size <= 0) return 0;

            if (usePtrace)
                attach(pid);

            std::filesystem::path path = std::format("/proc/{}/mem", pid);
            auto fd = open(path.c_str(), O_RDWR);

            ssize_t bytesRead = pread64(fd, &buffer, size, address);

            close(fd);

            if (usePtrace)
                detach(pid);

            return bytesRead;
        }

        ssize_t write(pid_t pid, uintptr_t address, const void* buffer, size_t size, bool usePtrace)
        {
            if (pid <= 0 || address <= 0 || size <= 0) return 0;

            if (usePtrace)
                attach(pid);

            std::filesystem::path path = std::format("/proc/{}/mem", pid);
            auto fd = open(path.c_str(), O_RDWR);

            ssize_t bytesWritten = pwrite64(fd, &buffer, size, address);

            close(fd);

            if (usePtrace)
                detach(pid);

            return bytesWritten;
        }
    } // namespace io

    namespace sys {
        ssize_t read(pid_t pid, uintptr_t address, void* buffer, size_t size)
        {
            if (pid <= 0 || address <= 0 || size <= 0) return 0;

            struct iovec local = { .iov_base = buffer, .iov_len = size };
            struct iovec remote = { .iov_base = reinterpret_cast<void*>(address), .iov_len = size };

            ssize_t bytesRead = process_vm_readv(pid, &local, 1, &remote, 1, 0);

            return bytesRead;
        }

        ssize_t write(pid_t pid, uintptr_t address, const void* buffer, size_t size)
        {
            if (pid <= 0 || address <= 0 || size <= 0) return 0;

            struct iovec local = { .iov_base = const_cast<void*>(buffer), .iov_len = size };
            struct iovec remote = { .iov_base = reinterpret_cast<void*>(address), .iov_len = size };

            ssize_t bytesWritten = process_vm_writev(pid, &local, 1, &remote, 1, 0);

            return bytesWritten;
        }
    } // namespace sys
}
