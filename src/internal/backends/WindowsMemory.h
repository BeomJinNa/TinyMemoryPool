#pragma once

#if defined(_WIN32)

#include "Common.h"

#include <Windows.h>

#include <cstddef>

namespace TinyMemoryPool::Detail
{

/// @brief Win32 VirtualAlloc/VirtualFree 기반 가상 메모리 백엔드.
/// 인스턴스 생성을 금지한 유틸리티 클래스.
class WindowsMemory final
{
  public:
    [[nodiscard]] static inline void* ReserveOrNull(std::size_t size) noexcept
    {
        return VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
    }

    static inline void Commit(void* ptr, std::size_t size) noexcept
    {
        void* result = VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
        if(result == nullptr)
        {
            TMP_FATAL_ERROR("VirtualAlloc commit failed!");
        }
    }

    static inline void Release(void* ptr, [[maybe_unused]] std::size_t size) noexcept
    {
        BOOL success = VirtualFree(ptr, 0, MEM_RELEASE);
        if(success == FALSE)
        {
            TMP_FATAL_ERROR("VirtualFree release failed!");
        }
    }

    static inline std::size_t GetPageSize() noexcept
    {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        return sysInfo.dwPageSize;
    }

  private:
    WindowsMemory() = delete;
    ~WindowsMemory() = delete;
};

} // namespace TinyMemoryPool::Detail

#endif
