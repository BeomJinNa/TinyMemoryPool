#pragma once

#if !defined(_WIN32)

#include "Common.h"

#include <sys/mman.h>
#include <unistd.h>

#include <cstddef>

namespace TinyMemoryPool::Detail
{

/// @brief mmap으로 영역을 확보하고 mprotect로 접근 권한을 바꾸며 munmap으로 해제한다.
class PosixMemory final
{
  public:
    [[nodiscard]] static inline void* ReserveOrNull(std::size_t size) noexcept
    {
        void* ptr = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        return (ptr == MAP_FAILED) ? nullptr : ptr;
    }

    static inline void Commit(void* ptr, std::size_t size) noexcept
    {
        int result = mprotect(ptr, size, PROT_READ | PROT_WRITE);
        if(result != 0)
        {
            TMP_FATAL_ERROR("mprotect failed!");
        }
    }

    static inline void Release(void* ptr, std::size_t size) noexcept
    {
        int result = munmap(ptr, size);
        if(result != 0)
        {
            TMP_FATAL_ERROR("munmap failed!");
        }
    }

    static inline std::size_t GetPageSize() noexcept
    {
        return static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
    }

  private:
    PosixMemory() = delete;
    ~PosixMemory() = delete;
};

} // namespace TinyMemoryPool::Detail

#endif
