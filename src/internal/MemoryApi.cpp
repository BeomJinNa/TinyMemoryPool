#include <TinyMemoryPool/Detail/MemoryApi.h>

#include "PoolManager.h"

namespace TinyMemoryPool::Detail
{

void* EngineAllocate(std::size_t size)
{
    return PoolManager::GetInstance().Allocate(size);
}

void EngineDeallocate(void* ptr, [[maybe_unused]] std::size_t size)
{
    PoolManager::GetInstance().Deallocate(ptr);
}

} // namespace TinyMemoryPool::Detail
