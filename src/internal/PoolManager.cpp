#include "PoolManager.h"
#include "Common.h"
#include "MemoryManager.h"
#include "Pool.h"

#include <algorithm>
#include <bit>
#include <cstdlib>
#include <memory>

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

        // 작은 청크일수록 초기 확보량을 늘려 Hot Path에서의 Grow 호출을 줄임
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
        // 4KB 초과 시 시스템 할당으로 fallback
        void* block = std::malloc(totalSize);
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
        std::free(header);
    }
}

[[nodiscard]] std::size_t PoolManager::GetPoolIndex(std::size_t totalSize) const
{
    // 64B 미만 요청도 최소 64B 풀(index 0)로 라우팅
    const std::size_t clampedSize = std::max(totalSize, static_cast<std::size_t>(1 << MIN_BIT_SHIFT));

    // bit_width: C++20 <bit>. 대부분 BSR/LZCNT 하드웨어 명령어로 변환됨.
    return std::bit_width(clampedSize - 1) - MIN_BIT_SHIFT;
}

} // namespace TinyMemoryPool::Detail
