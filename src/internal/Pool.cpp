#include "Pool.h"

#include <cstddef>

#include "Common.h"
#include "MemoryManager.h"

namespace TinyMemoryPool::Detail
{

void Pool::Initialize(std::size_t chunkSize, std::size_t initialBlockSize)
{
    mChunkSize = chunkSize;
    mNextBlockSize = initialBlockSize;

    Grow();
}

void Pool::Shutdown() noexcept
{
    // 큐 내부 노드만 정리. 실제 메모리 블록은 MemoryManager가 소유/해제함.
    mFreeList.clear();
}

[[nodiscard]] void* Pool::Pop()
{
    void* ptr = nullptr;

    // [Hot Path] 락 없이 즉시 할당 시도
    if(mFreeList.try_pop(ptr))
    {
        return ptr;
    }

    // [Cold Path] 큐가 비었을 경우 확장 시도
    if(Grow())
    {
        if(mFreeList.try_pop(ptr))
        {
            return ptr;
        }
    }

    TMP_FATAL_ERROR("Failed to pop from pool after growing.");
    return nullptr;
}

void Pool::Push(void* ptr)
{
    mFreeList.push(ptr);
}

std::size_t Pool::GetChunkSize() const noexcept
{
    return mChunkSize;
}

bool Pool::Grow()
{
    std::lock_guard<std::mutex> lock(mGrowMutex);

    // Double-Checked Locking: 락 대기 중 다른 스레드가 이미 확장했을 수 있음
    if(!mFreeList.empty())
    {
        return true;
    }

    void* newBlock = ::TinyMemoryPool::MemoryManager::GetInstance().AllocateBlock(mNextBlockSize);

    // 블록을 청크 단위로 분할(Slicing)하여 큐에 투입
    const std::size_t numChunks = mNextBlockSize / mChunkSize;
    auto currentChunk = static_cast<std::byte*>(newBlock);

    for(std::size_t i = 0; i < numChunks; ++i)
    {
        mFreeList.push(currentChunk);
        currentChunk += mChunkSize;
    }

    mNextBlockSize *= 2;

    return true;
}

} // namespace TinyMemoryPool::Detail