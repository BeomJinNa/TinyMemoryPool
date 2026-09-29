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

/// @brief 운영체제별 가상 메모리 처리를 공통 인터페이스로 제공한다.
/// @note 예약 실패 시 진단을 출력하고 종료한다. 커밋·해제 실패는 각 백엔드에서 처리한다.
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
