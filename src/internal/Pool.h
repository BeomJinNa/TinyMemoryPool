#pragma once

#include <cstddef>
#include <mutex>

#include <tbb/concurrent_queue.h>

namespace TinyMemoryPool::Detail
{

/**
 * @class Pool
 * @brief 단일 크기의 메모리 청크(Chunk)들을 관리하는 Lock-Free 기반(부분적) 풀 클래스.
 * @note Intel TBB의 concurrent_queue를 사용하여 대부분의 할당/해제 시 락 경합이 없음.
 */
class Pool
{
  public:
    Pool() = default;
    ~Pool() = default;

    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;

    /**
     * @brief 풀을 초기화하고 첫 메모리 블록을 할당합니다.
     * @param chunkSize 관리할 청크의 크기 (Byte)
     * @param initialBlockSize 최초 할당할 블록의 크기 (Byte)
     */
    void Initialize(std::size_t chunkSize, std::size_t initialBlockSize);

    /**
     * @brief 풀을 종료하고 내부 큐를 정리합니다.
     * 실제 메모리 해제는 MemoryManager가 프로그램 종료 시 일괄 수행합니다.
     */
    void Shutdown() noexcept;

    /**
     * @brief 가용 청크를 하나 가져옵니다. (Thread-Safe)
     * @return 유효한 메모리 주소 (실패 시 프로그램 종료)
     */
    [[nodiscard]] void* Pop();

    /**
     * @brief 사용한 청크를 반납합니다. (Thread-Safe)
     */
    void Push(void* ptr);

    /**
     * @brief 이 풀이 관리하는 청크의 크기를 반환합니다.
     */
    std::size_t GetChunkSize() const noexcept;

  private:
    /**
     * @brief 가용 청크가 없을 때 MemoryManager로부터 새 블록을 할당받아 확장합니다.
     * @note 내부적으로 Mutex를 사용하며 Double-Checked Locking으로 보호됩니다.
     */
    bool Grow();

  private:
    std::size_t mChunkSize = 0;
    std::size_t mNextBlockSize = 0;

    tbb::concurrent_queue<void*> mFreeList;

    // 확장 시 동기화를 위한 뮤텍스 (Cold Path 전용)
    std::mutex mGrowMutex;
};

} // namespace TinyMemoryPool::Detail