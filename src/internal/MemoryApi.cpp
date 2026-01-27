#include <TinyMemoryPool/Detail/MemoryApi.h>

#include "PoolManager.h"

namespace TinyMemoryPool::Detail
{
void* EngineAllocate(std::size_t size)
{
    return PoolManager::GetInstance().Allocate(size);
}

void EngineDeallocate(void* ptr, std::size_t /*size*/) // size는 내부에서 안 쓸 경우 주석 처리
{
    PoolManager::GetInstance().Deallocate(ptr);
}

} // namespace TinyMemoryPool::Detail