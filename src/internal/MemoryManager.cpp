#include "MemoryManager.h"
#include "Common.h"
#include "PlatformMemory.h"

#include <new>

namespace TinyMemoryPool
{

MemoryManager& MemoryManager::GetInstance()
{
    static MemoryManager instance;
    return instance;
}

MemoryManager::~MemoryManager()
{
    Shutdown();
}

void MemoryManager::Initialize(const MemoryManagerConfig& config)
{
    std::lock_guard<std::mutex> lock(mMutex);

    if(mIsInitialized)
    {
        return;
    }

    mTotalReservedSize = config.TotalReserveSize;
    mReservedBaseAddress = Detail::PlatformMemory::Reserve(mTotalReservedSize);
    mPageSize = Detail::PlatformMemory::GetPageSize();

    mCurrentCommitOffset = 0;
    mIsInitialized = true;
}

void MemoryManager::Shutdown() noexcept
{
    std::lock_guard<std::mutex> lock(mMutex);

    if(!mIsInitialized)
    {
        return;
    }

    if(mReservedBaseAddress)
    {
        Detail::PlatformMemory::Release(mReservedBaseAddress, mTotalReservedSize);
    }

    mReservedBaseAddress = nullptr;
    mTotalReservedSize = 0;
    mCurrentCommitOffset = 0;
    mPageSize = 0;
    mIsInitialized = false;
}

[[nodiscard]] void* MemoryManager::AllocateBlock(std::size_t size)
{
    std::lock_guard<std::mutex> lock(mMutex);

    TMP_ASSERT(mIsInitialized && "MemoryManager is not initialized.");

    const std::size_t pageSize = mPageSize;
    TMP_ASSERT((pageSize & (pageSize - 1)) == 0);

    // 페이지 정렬: 요청 크기를 페이지 경계로 올림 (비트 마스크 방식)
    const std::size_t alignedSize = (size + pageSize - 1) & ~(pageSize - 1);

    if(mCurrentCommitOffset + alignedSize > mTotalReservedSize)
    {
        TMP_FATAL_ERROR("Out of reserved memory (MemoryManager). Increase Reserve Size.");
        return nullptr;
    }

    auto* basePtr = static_cast<std::byte*>(mReservedBaseAddress);
    void* commitAddress = basePtr + mCurrentCommitOffset;

    Detail::PlatformMemory::Commit(commitAddress, alignedSize);

    mCurrentCommitOffset += alignedSize;

    return commitAddress;
}

} // namespace TinyMemoryPool
