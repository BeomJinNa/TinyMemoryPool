#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace TinyMemoryPool
{

/// @brief 개별 풀의 청크/블록 크기 설정.
struct PoolConfig
{
    std::size_t ChunkSize = 0;
    std::size_t InitialBlockSize = 0;
};

/// @brief MemoryManager 전체 초기화 설정.
struct MemoryManagerConfig
{
    std::size_t TotalReserveSize = 1024 * 1024 * 1024;
    std::size_t FrameAllocatorSize = 16 * 1024 * 1024;
    std::vector<PoolConfig> PoolConfigs;
};

} // namespace TinyMemoryPool
