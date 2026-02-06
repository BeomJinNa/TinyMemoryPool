#pragma once

#include <cstddef>

namespace TinyMemoryPool::Detail
{

/// @brief PoolManager 싱글턴으로 연결되는 할당 브릿지 함수.
/// @note LTO(Link Time Optimization)를 통해 최종 바이너리에서 인라인 처리된다.
[[nodiscard]] void* EngineAllocate(std::size_t size);

void EngineDeallocate(void* ptr, std::size_t size);

} // namespace TinyMemoryPool::Detail
