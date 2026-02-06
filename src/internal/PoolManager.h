#pragma once

#include <TinyMemoryPool/Config.h>

#include "Common.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace TinyMemoryPool::Detail
{

class Pool;

/// @brief 메모리 블록 앞단의 메타데이터 (16 Bytes).
/// @note 16바이트 정렬 보장을 위해 패딩 없이 구성됨.
struct BlockHeader
{
    Pool* OwnerPool;    ///< 소유 Pool. nullptr이면 System Malloc으로 할당됨.
    std::size_t Size;   ///< 전체 할당 크기 (Header 포함).
};

/// @brief 요청 크기에 따라 적절한 Pool로 라우팅하는 중앙 관리자.
/// Meyers Singleton. Bit Scan 기반 O(1) 라우팅.
class PoolManager final
{
  public:
    static PoolManager& GetInstance();

    void Initialize();
    void Shutdown();

    /// @brief 스레드 안전한 메모리 할당.
    /// @param size 사용자 요청 크기 (Byte).
    [[nodiscard]] void* Allocate(std::size_t size);

    /// @brief 스레드 안전한 메모리 해제.
    /// @param ptr Allocate로 할당받은 메모리 주소.
    void Deallocate(void* ptr);

  private:
    PoolManager();
    ~PoolManager();

    PoolManager(const PoolManager&) = delete;
    PoolManager& operator=(const PoolManager&) = delete;

    [[nodiscard]] std::size_t GetPoolIndex(std::size_t totalSize) const;

  private:
    std::vector<std::unique_ptr<Pool>> mPools;
    bool mIsInitialized = false;

    static constexpr std::size_t MIN_BIT_SHIFT = 6;      ///< 최소 청크 64B = 2^6.
    static constexpr std::size_t MAX_BLOCK_SIZE = 4096;   ///< 이 크기 초과 시 System Malloc fallback.
    static constexpr std::size_t POOL_COUNT = 7;          ///< 64, 128, 256, 512, 1024, 2048, 4096.
};

} // namespace TinyMemoryPool::Detail
