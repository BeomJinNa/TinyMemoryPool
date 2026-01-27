#include "PoolManager.h"
#include "Common.h"
#include "MemoryManager.h"
#include "Pool.h"

#include <algorithm>
#include <bit>
#include <cstdlib>

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

// [초기화] Meyers Singleton: 최초 호출 시 생성자 실행됨 -> 자동 초기화
PoolManager& PoolManager::GetInstance()
{
    static PoolManager instance;
    return instance;
}

PoolManager::PoolManager()
{
    // 1. 하위 계층(MemoryManager) 먼저 초기화 (기본 설정)
    MemoryManagerConfig config;
    // config.totalReserveSize = ...; // 필요하면 기본값 수정
    MemoryManager::GetInstance().Initialize(config);

    // 2. 자기 자신(PoolManager) 초기화
    Initialize();
}

PoolManager::~PoolManager()
{
    Shutdown();
    // MemoryManager는 자신의 소멸자에서 알아서 꺼짐
}

void PoolManager::Initialize()
{
    if(mIsInitialized)
        return;

    mPools.reserve(POOL_COUNT);

    // 64B 부터 시작
    std::size_t currentChunkSize = (1 << MIN_BIT_SHIFT);

    for(std::size_t i = 0; i < POOL_COUNT; ++i)
    {
        auto* newPool = new Pool();

        // 작은 블록일수록 더 많은 개수를 초기에 확보 (Tiered Strategy)
        std::size_t initialItemCount = (currentChunkSize <= 256) ? 4096 : (currentChunkSize <= 1024) ? 1024 : 256;

        newPool->Initialize(currentChunkSize, currentChunkSize * initialItemCount);
        mPools.push_back(newPool);

        currentChunkSize *= 2;
    }

    mIsInitialized = true;
}

void PoolManager::Shutdown()
{
    if(!mIsInitialized)
        return;

    for(auto* pool : mPools)
    {
        if(pool)
        {
            pool->Shutdown();
            delete pool;
        }
    }
    mPools.clear();
    mIsInitialized = false;
}

[[nodiscard]] void* PoolManager::Allocate(std::size_t size)
{
    // 헤더 크기 포함
    const std::size_t totalSize = size + sizeof(BlockHeader);

    BlockHeader* header = nullptr;

    // 1. Routing (Fast Path)
    if(totalSize <= MAX_BLOCK_SIZE)
    {
        // [최적화] Branchless Index Calculation
        const std::size_t index = GetPoolIndex(totalSize);

        // 인덱스 범위 안전장치 (릴리즈에선 제거됨)
        TMP_ASSERT(index < mPools.size());

        void* block = mPools[index]->Pop(); // 여기서 MemoryManager 할당 발생 가능

        // Pop 실패 시(매우 드뭄) nullptr 체크는 Pool 내부나 여기서 해야 함
        if(!block) [[unlikely]]
            return nullptr;

        header = static_cast<BlockHeader*>(block);
        header->pOwnerPool = mPools[index];
    }
    else
    {
        // 2. Fallback (System Malloc)
        // 4KB 초과 시 시스템 할당 사용
        void* block = std::malloc(totalSize);
        if(!block) [[unlikely]]
            return nullptr;

        header = static_cast<BlockHeader*>(block);
        header->pOwnerPool = nullptr; // 소유자 없음 표시
    }

    // 디버깅용 사이즈 기록
    header->size = totalSize;

    return GetPayloadAddress(header);
}

void PoolManager::Deallocate(void* ptr)
{
    if(ptr == nullptr)
        return;

    // 헤더 역추적
    BlockHeader* header = GetHeaderAddress(ptr);

    if(header->pOwnerPool)
    {
        // 풀에 반납 (Lock-Free Push)
        header->pOwnerPool->Push(header);
    }
    else
    {
        // 시스템 반납
        std::free(header);
    }
}

[[nodiscard]] std::size_t PoolManager::GetPoolIndex(std::size_t totalSize) const
{
    // [Branchless Logic]
    // 64바이트 미만 요청이 와도 64바이트 풀(Index 0)로 보내야 함.
    const std::size_t clampedSize = std::max(totalSize, (std::size_t) (1 << MIN_BIT_SHIFT));

    // bit_width는 C++20의 <bit> 헤더 기능 (대부분 하드웨어 명령어로 변환됨)
    // 예: 64 -> bit_width(63) = 6 -> 6 - 6 = 0
    return std::bit_width(clampedSize - 1) - MIN_BIT_SHIFT;
}

} // namespace TinyMemoryPool::Detail