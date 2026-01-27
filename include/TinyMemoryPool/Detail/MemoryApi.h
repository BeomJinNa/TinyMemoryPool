#pragma once

#include <cstddef>

namespace TinyMemoryPool::Detail
{
/**
 * @brief 내부 엔진(PoolManager)으로 연결되는 브릿지 함수들.
 * @note 이 함수들은 cpp 내부에서 PoolManager 싱글턴을 호출하며,
 * LTO(Link Time Optimization)를 통해 최종적으로 인라인 처리
 */
[[nodiscard]] void* EngineAllocate(std::size_t size);

void EngineDeallocate(void* ptr, std::size_t size);

} // namespace TinyMemoryPool::Detail