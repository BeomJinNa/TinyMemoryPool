#include "MemoryManager.h"

#include <new> // std::bad_alloc

#include "Common.h"
#include "PlatformMemory.h"

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
        // 이미 초기화되었으면 에러보다는 무시하거나 로그
        return;
    }

    mTotalReservedSize = config.totalReserveSize;

    // 1. 가상 메모리 예약 (Commit 아님)
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

    // [페이지 정렬 로직]
    // size가 13KB이고 Page가 4KB라면 -> 16KB(4페이지)를 할당해야 함.
    // (size + pageSize - 1) & ~(pageSize - 1) 공식 사용
    const std::size_t pageSize = mPageSize;

    // 안전장치: pageSize가 0이거나 2의 거듭제곱이 아니면 비트 연산 불가
    // 보통 OS 페이지는 4096(2^12)이므로 안전하지만 assert 추가 권장
    TMP_ASSERT((pageSize & (pageSize - 1)) == 0);

    const std::size_t alignedSize = (size + pageSize - 1) & ~(pageSize - 1);

    // 1. 예약 공간 초과 확인
    if(mCurrentCommitOffset + alignedSize > mTotalReservedSize)
    {
        TMP_FATAL_ERROR("Out of reserved memory (MemoryManager). Increase Reserve Size.");
        return nullptr;
    }

    // 2. 커밋할 주소 계산
    // void*는 산술 연산이 불가능하므로 std::byte*로 변환 후 오프셋 더하기
    auto* basePtr = static_cast<std::byte*>(mReservedBaseAddress);
    void* commitAddress = basePtr + mCurrentCommitOffset;

    // 3. 실제 물리 메모리 매핑 (Commit)
    Detail::PlatformMemory::Commit(commitAddress, alignedSize);

    mCurrentCommitOffset += alignedSize;

    return commitAddress;
}

} // namespace TinyMemoryPool