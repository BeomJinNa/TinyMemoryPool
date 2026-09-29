#include "PoolManager.h"
#include "Common.h"
#include "MemoryManager.h"
#include "Pool.h"

#include <algorithm>
#include <bit>
#include <limits>
#include <memory>
#include <new>

namespace
{

using namespace TinyMemoryPool::Detail;

[[nodiscard]] inline void* GetPayloadAddress(BlockHeader* header)
{
    return header + 1;
}

[[nodiscard]] inline BlockHeader* GetHeaderAddress(void* payload)
{
    return static_cast<BlockHeader*>(payload) - 1;
}

} // namespace

namespace TinyMemoryPool::Detail
{

PoolManager& PoolManager::GetInstance()
{
    static PoolManager instance;
    return instance;
}

PoolManager::PoolManager()
{
    MemoryManagerConfig config;
    MemoryManager::GetInstance().Initialize(config);

    Initialize();
}

PoolManager::~PoolManager()
{
    Shutdown();
}

void PoolManager::Initialize()
{
    if(mIsInitialized)
        return;

    mPools.reserve(POOL_COUNT);

    std::size_t currentChunkSize = (1 << MIN_BIT_SHIFT);

    for(std::size_t i = 0; i < POOL_COUNT; ++i)
    {
        auto newPool = std::make_unique<Pool>();

        // 작은 크기의 풀은 청크를 더 많이 확보해 초기 확장 빈도를 줄인다.
        std::size_t initialItemCount = (currentChunkSize <= 256) ? 4096 : (currentChunkSize <= 1024) ? 1024 : 256;

        newPool->Initialize(currentChunkSize, currentChunkSize * initialItemCount);
        mPools.push_back(std::move(newPool));

        currentChunkSize *= 2;
    }

    mIsInitialized = true;
}

void PoolManager::Shutdown()
{
    if(!mIsInitialized)
        return;

    for(auto& pool : mPools)
    {
        if(pool)
        {
            pool->Shutdown();
        }
    }
    mPools.clear();
    mIsInitialized = false;
}

[[nodiscard]] void* PoolManager::Allocate(std::size_t size)
{
    if(size > std::numeric_limits<std::size_t>::max() - sizeof(BlockHeader))
        return nullptr;

    const std::size_t totalSize = size + sizeof(BlockHeader);

    BlockHeader* header = nullptr;

    if(totalSize <= MAX_BLOCK_SIZE)
    {
        const std::size_t index = GetPoolIndex(totalSize);
        TMP_ASSERT(index < mPools.size());

        void* block = mPools[index]->Pop();
        if(!block) [[unlikely]]
            return nullptr;

        header = static_cast<BlockHeader*>(block);
        header->OwnerPool = mPools[index].get();
    }
    else
    {
        // 풀 상한보다 큰 요청도 반환 주소의 기본 정렬은 동일하게 유지한다.
        void* block = ::operator new(totalSize, std::align_val_t{PoolAlignment}, std::nothrow);
        if(!block) [[unlikely]]
            return nullptr;

        header = static_cast<BlockHeader*>(block);
        header->OwnerPool = nullptr;
    }

    header->Size = totalSize;

    return GetPayloadAddress(header);
}

void PoolManager::Deallocate(void* ptr)
{
    if(ptr == nullptr)
        return;

    BlockHeader* header = GetHeaderAddress(ptr);

    if(header->OwnerPool)
    {
        header->OwnerPool->Push(header);
    }
    else
    {
        ::operator delete(header, std::align_val_t{PoolAlignment});
    }
}

[[nodiscard]] std::size_t PoolManager::GetPoolIndex(std::size_t totalSize) const
{
    // 헤더를 포함한 크기가 64바이트보다 작아도 첫 번째 풀을 사용한다.
    const std::size_t clampedSize = std::max(totalSize, static_cast<std::size_t>(1 << MIN_BIT_SHIFT));

    // 요청 크기를 담을 수 있는 가장 작은 2의 거듭제곱 크기 풀을 선택한다.
    return std::bit_width(clampedSize - 1) - MIN_BIT_SHIFT;
}

} // namespace TinyMemoryPool::Detail
