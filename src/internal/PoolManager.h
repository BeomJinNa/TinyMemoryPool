#pragma once

#include <cstddef>
#include <vector>

#include <TinyMemoryPool/Config.h>

#include "Common.h"

namespace TinyMemoryPool::Detail
{

class Pool;

/**
 * @struct BlockHeader
 * @brief 메모리 블록 앞단의 메타데이터 (16 Bytes).
 * @note 16바이트 정렬(Alignment)을 보장하기 위해 Padding 없이 구성됨.
 */
struct BlockHeader
{
    Pool* pOwnerPool; // 소유 Pool (nullptr이면 System Malloc)
    std::size_t size; // 전체 할당 크기 (Header 포함)
};

/**
 * @class PoolManager
 * @brief 요청 크기에 따라 적절한 Block Pool로 라우팅하는 중앙 관리자.
 * @note 싱글턴 패턴 적용 / Bit Scan 기반 O(1) 라우팅.
 */
class PoolManager
{
  public:
    static PoolManager& GetInstance();

    void Initialize();
    void Shutdown();

    /**
     * @brief 스레드 안전한 메모리 할당.
     * @param size 사용자 요청 크기 (Byte)
     */
    [[nodiscard]] void* Allocate(std::size_t size);

    /**
     * @brief 스레드 안전한 메모리 해제.
     * @param ptr Allocate로 할당받은 메모리 주소
     */
    void Deallocate(void* ptr);

  private:
    PoolManager();
    ~PoolManager();

    PoolManager(const PoolManager&) = delete;
    PoolManager& operator=(const PoolManager&) = delete;

    [[nodiscard]] std::size_t GetPoolIndex(std::size_t totalSize) const;

  private:
    std::vector<Pool*> mPools;
    bool mIsInitialized = false;

    // Routing Constants (64B ~ 4096B)
    static constexpr std::size_t MIN_BIT_SHIFT = 6;
    static constexpr std::size_t MAX_BLOCK_SIZE = 4096;
    static constexpr std::size_t POOL_COUNT = 7;
};

} // namespace TinyMemoryPool::Detail