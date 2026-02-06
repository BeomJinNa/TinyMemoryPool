#pragma once

#include "Common.h"

#include <cstddef>

#if defined(_WIN32)
#include "backends/WindowsMemory.h"
#define PLATFORM_MEMORY_BACKEND TinyMemoryPool::Detail::WindowsMemory
#else
#include "backends/PosixMemory.h"
#define PLATFORM_MEMORY_BACKEND TinyMemoryPool::Detail::PosixMemory
#endif

namespace TinyMemoryPool::Detail
{

/// @brief 플랫폼별 가상 메모리 API를 통합하는 파사드 클래스.
/// Reserve 실패 시 TMP_FATAL_ERROR로 즉시 종료한다.
class PlatformMemory final
{
  public:
    [[nodiscard]] static inline void* Reserve(std::size_t size) noexcept
    {
        void* ptr = PLATFORM_MEMORY_BACKEND::ReserveOrNull(size);
        if(ptr == nullptr)
        {
            TMP_FATAL_ERROR("PlatformMemory::Reserve failed!");
        }
        return ptr;
    }

    static inline void Commit(void* ptr, std::size_t size) noexcept { PLATFORM_MEMORY_BACKEND::Commit(ptr, size); }

    static inline void Release(void* ptr, std::size_t size) noexcept { PLATFORM_MEMORY_BACKEND::Release(ptr, size); }

    static inline std::size_t GetPageSize() noexcept { return PLATFORM_MEMORY_BACKEND::GetPageSize(); }

  private:
    PlatformMemory() = delete;
    ~PlatformMemory() = delete;
};

} // namespace TinyMemoryPool::Detail
