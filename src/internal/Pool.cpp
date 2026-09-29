#include "Pool.h"
#include "Common.h"
#include "MemoryManager.h"

#include <cstddef>
#include <limits>

namespace TinyMemoryPool::Detail
{

void Pool::Initialize(std::size_t chunkSize, std::size_t initialBlockSize)
{
    if(chunkSize == 0 || initialBlockSize < chunkSize || initialBlockSize % chunkSize != 0)
    {
        TMP_FATAL_ERROR("Invalid pool block size.");
    }

    mChunkSize = chunkSize;
    mNextBlockSize = initialBlockSize;

    // 초기화에서는 호출자용 청크도 반납하여 첫 블록 전체를 가용 상태로 둔다.
    Push(GrowAndPop());
}

void Pool::Shutdown() noexcept
{
    // 큐만 비운다. 청크의 원본 메모리는 MemoryManager가 소유하고 해제한다.
    mFreeList.clear();
}

[[nodiscard]] void* Pool::Pop()
{
    void* ptr = nullptr;

    if(mFreeList.try_pop(ptr))
    {
        return ptr;
    }

    return GrowAndPop();
}

void Pool::Push(void* ptr)
{
    mFreeList.push(ptr);
}

std::size_t Pool::GetChunkSize() const noexcept
{
    return mChunkSize;
}

void* Pool::GrowAndPop()
{
    std::lock_guard<std::mutex> lock(mGrowMutex);

    // 락을 기다리는 동안 청크가 반납되거나 풀이 확장됐을 수 있으므로 다시 확인한다.
    void* ptr = nullptr;
    if(mFreeList.try_pop(ptr))
    {
        return ptr;
    }

    void* newBlock = ::TinyMemoryPool::MemoryManager::GetInstance().AllocateBlock(mNextBlockSize);

    const std::size_t numChunks = mNextBlockSize / mChunkSize;
    auto* reservedChunk = static_cast<std::byte*>(newBlock);
    auto* currentChunk = reservedChunk + mChunkSize;

    // 다른 스레드는 이 락 없이 청크를 가져가므로, 호출자 몫 하나는 큐에 넣지 않는다.
    for(std::size_t i = 1; i < numChunks; ++i)
    {
        mFreeList.push(currentChunk);
        currentChunk += mChunkSize;
    }

    if(mNextBlockSize <= std::numeric_limits<std::size_t>::max() / 2)
    {
        mNextBlockSize *= 2;
    }

    return reservedChunk;
}

} // namespace TinyMemoryPool::Detail
