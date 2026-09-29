#pragma once

#include <tbb/concurrent_queue.h>

#include <cstddef>
#include <mutex>

namespace TinyMemoryPool::Detail
{

/// @brief TBB concurrent_queue로 가용 청크를 관리하는 고정 크기 풀.
/// @note 초기화 후 Pop·Push는 동시 호출할 수 있다. 초기화·종료와 사용은 겹치지 않아야 한다.
class Pool final
{
  public:
    Pool() = default;
    ~Pool() = default;

    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;

    /// @brief 풀을 초기화하고 첫 메모리 블록을 할당한다.
    /// @param chunkSize 청크 하나의 크기(바이트).
    /// @param initialBlockSize 첫 블록의 크기(바이트). 청크 크기의 양의 배수여야 한다.
    void Initialize(std::size_t chunkSize, std::size_t initialBlockSize);

    /// @brief 풀을 종료하고 내부 큐를 정리한다.
    /// @note 모든 사용이 끝난 뒤 호출한다. 원본 메모리는 MemoryManager가 일괄 해제한다.
    void Shutdown() noexcept;

    /// @brief 가용 청크를 하나 꺼낸다.
    /// @return 유효한 청크 주소. 메모리 확보에 실패하면 프로그램을 종료한다.
    [[nodiscard]] void* Pop();

    /// @brief 사용 완료된 청크를 반납한다.
    void Push(void* ptr);

    std::size_t GetChunkSize() const noexcept;

  private:
    /// @brief 락 획득 후 큐를 다시 확인하고, 필요하면 확장하여 청크 하나를 확보한다.
    /// @note 새 블록의 첫 청크는 공유 큐에 넣지 않고 반환한다.
    [[nodiscard]] void* GrowAndPop();

  private:
    std::size_t mChunkSize = 0;
    std::size_t mNextBlockSize = 0;

    tbb::concurrent_queue<void*> mFreeList;

    std::mutex mGrowMutex; ///< 첫 try_pop이 실패한 경로에서만 획득한다.
};

} // namespace TinyMemoryPool::Detail
