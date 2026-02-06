#pragma once

#include <tbb/concurrent_queue.h>

#include <cstddef>
#include <mutex>

namespace TinyMemoryPool::Detail
{

/// @brief 단일 크기의 메모리 청크들을 관리하는 Lock-Free 기반(부분적) 풀.
/// Intel TBB concurrent_queue를 사용하여 대부분의 할당/해제가 락 없이 동작한다.
class Pool final
{
  public:
    Pool() = default;
    ~Pool() = default;

    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;

    /// @brief 풀을 초기화하고 첫 메모리 블록을 할당한다.
    /// @param chunkSize 관리할 청크의 크기 (Byte).
    /// @param initialBlockSize 최초 할당할 블록의 크기 (Byte).
    void Initialize(std::size_t chunkSize, std::size_t initialBlockSize);

    /// @brief 풀을 종료하고 내부 큐를 정리한다.
    /// @note 실제 메모리 해제는 MemoryManager가 프로그램 종료 시 일괄 수행한다.
    void Shutdown() noexcept;

    /// @brief 가용 청크를 하나 꺼낸다 (Thread-Safe).
    /// @return 유효한 메모리 주소. 실패 시 TMP_FATAL_ERROR로 종료.
    [[nodiscard]] void* Pop();

    /// @brief 사용 완료된 청크를 반납한다 (Thread-Safe).
    void Push(void* ptr);

    std::size_t GetChunkSize() const noexcept;

  private:
    /// @brief 가용 청크 소진 시 MemoryManager로부터 새 블록을 받아 확장한다.
    /// @note Double-Checked Locking으로 중복 확장을 방지한다.
    bool Grow();

  private:
    std::size_t mChunkSize = 0;
    std::size_t mNextBlockSize = 0;

    tbb::concurrent_queue<void*> mFreeList;

    std::mutex mGrowMutex; ///< 확장(Grow) 시에만 사용되는 Cold Path 뮤텍스.
};

} // namespace TinyMemoryPool::Detail
