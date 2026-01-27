#pragma once

#include <cstddef>
#include <mutex>

#include <TinyMemoryPool/Config.h>

namespace TinyMemoryPool
{
/**
 * @class MemoryManager
 * @brief OS로부터 가상 메모리를 예약하고 관리하는 중앙 관리자.
 */
class MemoryManager
{
  public:
    static MemoryManager& GetInstance();

    void Initialize(const MemoryManagerConfig& config);
    void Shutdown() noexcept;

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