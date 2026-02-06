#pragma once

#include <TinyMemoryPool/Config.h>

#include <cstddef>
#include <mutex>

namespace TinyMemoryPool
{

/// @brief OS로부터 가상 메모리를 예약(Reserve)하고 커밋(Commit) 단위로 분배하는 중앙 관리자.
/// 싱글턴 패턴 적용. 프로그램 종료 시 예약 메모리를 일괄 해제한다.
class MemoryManager final
{
  public:
    static MemoryManager& GetInstance();

    void Initialize(const MemoryManagerConfig& config);
    void Shutdown() noexcept;

    /// @brief 페이지 정렬된 메모리 블록을 커밋하여 반환한다.
    /// @param size 요청 크기 (내부에서 페이지 단위로 올림 정렬됨).
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
