#pragma once

#include <TinyMemoryPool/Config.h>
#include <TinyMemoryPool/Detail/MemoryApi.h>

#include "Common.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace TinyMemoryPool::Detail
{

class Pool;

/// @brief 반환할 데이터 영역 앞에 소유 풀과 할당 크기를 저장한다.
struct alignas(PoolAlignment) BlockHeader
{
    Pool* OwnerPool;    ///< 반납할 풀. nullptr이면 정렬 지정 시스템 할당으로 확보한 메모리다.
    std::size_t Size;   ///< 헤더를 포함한 전체 할당 크기.
};

/// @brief 요청 크기에 맞는 풀 또는 시스템 할당을 선택하는 공유 관리자.
/// @note 최초 사용 시 초기화되며, 요청 크기의 비트 수로 풀 인덱스를 계산한다.
class PoolManager final
{
  public:
    static PoolManager& GetInstance();

    void Initialize();
    /// @note 모든 사용 스레드와 풀 할당 객체의 수명이 끝난 뒤 호출한다.
    void Shutdown();

    /// @brief PoolAlignment를 만족하는 주소를 반환한다. 초기화 이후 동시 호출을 지원한다.
    /// @param size 사용자가 요청한 데이터 크기(바이트).
    [[nodiscard]] void* Allocate(std::size_t size);

    /// @brief 메모리 해제. 서로 다른 할당의 반납은 다른 할당·해제와 동시에 호출할 수 있다.
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
    static constexpr std::size_t MAX_BLOCK_SIZE = 4096;   ///< 이 크기 초과 시 정렬 지정 시스템 할당.
    static constexpr std::size_t POOL_COUNT = 7;          ///< 64, 128, 256, 512, 1024, 2048, 4096.

    static_assert((std::size_t{1} << MIN_BIT_SHIFT) % alignof(BlockHeader) == 0,
                  "Pool chunks must preserve the header alignment.");
};

} // namespace TinyMemoryPool::Detail
