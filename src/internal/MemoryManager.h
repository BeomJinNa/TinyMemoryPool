#pragma once

#include <TinyMemoryPool/Config.h>

#include <cstddef>
#include <mutex>

namespace TinyMemoryPool
{

/// @brief 가상 주소 영역을 예약하고 필요한 구간을 페이지 단위로 사용 가능하게 만든다.
/// @note 공유 인스턴스가 전체 예약 영역을 소유하며 소멸할 때 일괄 해제한다.
class MemoryManager final
{
  public:
    static MemoryManager& GetInstance();

    void Initialize(const MemoryManagerConfig& config);
    void Shutdown() noexcept;

    /// @brief 예약 영역의 다음 구간을 커밋하여 반환한다. 기존 블록은 이동하지 않는다.
    /// @param size 요청 크기(바이트). 내부에서 페이지 크기의 배수로 올림한다.
    [[nodiscard]] void* AllocateBlock(std::size_t size);

  private:
    MemoryManager() = default;
    ~MemoryManager();

    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

  private:
    std::mutex mMutex;
    bool mIsInitialized = false;

    void* mReservedBaseAddress = nullptr;

    std::size_t mCurrentCommitOffset = 0;
    std::size_t mTotalReservedSize = 0;
    std::size_t mPageSize = 0;
};

} // namespace TinyMemoryPool
