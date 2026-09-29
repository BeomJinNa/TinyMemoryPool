#include "MemoryManager.h"
#include "Common.h"
#include "PlatformMemory.h"

#include <TinyMemoryPool/Detail/MemoryApi.h>

#include <cstdint>
#include <limits>
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

    // 페이지 단위로 블록을 나눠도 각 블록의 시작 주소가 풀 정렬을 만족해야 한다.
    constexpr auto alignment = Detail::PoolAlignment;
    if(mPageSize == 0 || (mPageSize & (mPageSize - 1)) != 0 || mPageSize % alignment != 0 ||
       reinterpret_cast<std::uintptr_t>(mReservedBaseAddress) % alignment != 0)
    {
        TMP_FATAL_ERROR("Platform memory does not satisfy the pool alignment.");
    }

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

    if(size > (std::numeric_limits<std::size_t>::max)() - (pageSize - 1))
    {
        TMP_FATAL_ERROR("Block size overflow (MemoryManager).");
    }

    // 요청 크기를 페이지 크기의 배수로 올림한다.
    const std::size_t alignedSize = (size + pageSize - 1) & ~(pageSize - 1);

    if(alignedSize > mTotalReservedSize - mCurrentCommitOffset)
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
