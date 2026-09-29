#include <TinyMemoryPool/Detail/MemoryApi.h>

#include "Common.h"
#include "PoolManager.h"

namespace TinyMemoryPool::Detail
{

[[noreturn]] void AllocationFailure(const char* message) noexcept
{
    TMP_FATAL_ERROR(message);
}

void* EngineAllocate(std::size_t size)
{
    return PoolManager::GetInstance().Allocate(size);
}

void EngineDeallocate(void* ptr, [[maybe_unused]] std::size_t size)
{
    PoolManager::GetInstance().Deallocate(ptr);
}

} // namespace TinyMemoryPool::Detail
