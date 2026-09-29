#pragma once

#include <cstddef>

namespace TinyMemoryPool::Detail
{

inline constexpr std::size_t PoolAlignment = alignof(std::max_align_t) > 16 ? alignof(std::max_align_t) : 16;

[[noreturn]] void AllocationFailure(const char* message) noexcept;

/// @brief 내부 PoolManager에 할당을 요청하며 구현 헤더를 사용부에서 분리한다.
/// @note 반환 주소는 PoolAlignment를 만족한다. 풀 수명 내 할당·반납은 동시 호출할 수 있다.
[[nodiscard]] void* EngineAllocate(std::size_t size);

void EngineDeallocate(void* ptr, std::size_t size);

} // namespace TinyMemoryPool::Detail
