#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace TinyMemoryPool
{

/// @brief 개별 풀의 청크 크기와 초기 블록 크기를 담는 설정 값.
struct PoolConfig
{
    std::size_t ChunkSize = 0;
    std::size_t InitialBlockSize = 0;
};

/// @brief 가상 메모리 관리자의 초기화 설정.
/// @note 현재 구현은 TotalReserveSize만 사용하며 나머지 필드는 아직 연결하지 않는다.
struct MemoryManagerConfig
{
    std::size_t TotalReserveSize = 1024 * 1024 * 1024;
    std::size_t FrameAllocatorSize = 16 * 1024 * 1024;
    std::vector<PoolConfig> PoolConfigs;
};

} // namespace TinyMemoryPool
