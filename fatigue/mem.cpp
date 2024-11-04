#include <fcntl.h>
#include <filesystem>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <unistd.h>
#include "log.hpp"
#include "mem.hpp"
#include "proc.hpp"

namespace fatigue::mem {

    AccessMethod s_accessMethod{AccessMethod::SYS};

    void setAccessMethod(AccessMethod method) { s_accessMethod = method; }
    AccessMethod getAccessMethod() { return s_accessMethod; }

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

    namespace io {
        ssize_t read(pid_t pid, uintptr_t address, void* buffer, size_t size)
        {
            if (pid <= 0 || address <= 0 || size <= 0) return 0;

            std::filesystem::path path = std::format("/proc/{}/mem", pid);
            auto fd = open(path.c_str(), O_RDONLY);

            ssize_t bytesRead = pread64(fd, buffer, size, address);

            close(fd);

            return bytesRead;
        }

        ssize_t write(pid_t pid, uintptr_t address, const void* buffer, size_t size)
        {
            if (pid <= 0 || address <= 0 || size <= 0) return 0;

            std::filesystem::path path = std::format("/proc/{}/mem", pid);
            auto fd = open(path.c_str(), O_RDWR);

            ssize_t bytesWritten = pwrite64(fd, buffer, size, address);

            close(fd);

            return bytesWritten;
        }
    } // namespace io

    namespace trace {
        size_t read(pid_t pid, uintptr_t address, void* buffer, size_t size)
        {
            if (pid <= 0 || address <= 0 || size <= 0) return 0;

            size_t bytesRead = 0;

            while (bytesRead < size)
            {
                errno = 0;
                long data = ptrace(PTRACE_PEEKDATA, pid, address + bytesRead, 0);

                if (errno != 0)
                {
                    logError(std::format("mem::ptrace::read: failed to read from {:#x}: {}", address + bytesRead, strerror(errno)));
                    return bytesRead;
                }

                memcpy(static_cast<unsigned char*>(buffer) + bytesRead, &data, sizeof(data));
                bytesRead += sizeof(data);
            }

            return bytesRead;
        }

        size_t write(pid_t pid, uintptr_t address, const void* buffer, size_t size)
        {
            if (pid <= 0 || address <= 0 || size <= 0) return 0;

            size_t bytesWritten = 0;

            while (bytesWritten < size)
            {
                long data = 0;
                memcpy(&data, static_cast<const unsigned char*>(buffer) + bytesWritten, sizeof(data));

                errno = 0;
                ptrace(PTRACE_POKEDATA, pid, address + bytesWritten, data);

                if (errno != 0)
                {
                    logError(std::format("mem::ptrace::write: failed to write to {:#x}: {}", address + bytesWritten, errno));
                    return bytesWritten;
                }

                bytesWritten += sizeof(data);
            }

            return bytesWritten;
        }
    } // namespace ptrace
}
