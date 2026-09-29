#include "MemoryManager.h"
#include "Pool.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <semaphore>
#include <thread>
#include <unordered_set>
#include <vector>

namespace
{

std::binary_semaphore reachedReturn(0);
std::binary_semaphore resumeReturn(0);
std::atomic<std::size_t> growthCount{0};
thread_local bool pauseGrowth = false;
std::size_t grownChunks = 0;

void Require(bool condition, const char* message)
{
    if(!condition)
    {
        std::cerr << message << '\n';
        std::abort();
    }
}

} // namespace

namespace TinyMemoryPool::Detail
{

// 빌드 디렉터리에 생성한 테스트용 Pool.cpp 사본에서만 호출한다.
void TestAfterGrow(std::size_t numChunks)
{
    ++growthCount;
    if(pauseGrowth)
    {
        grownChunks = numChunks;
        reachedReturn.release();
        Require(resumeReturn.try_acquire_for(std::chrono::seconds(10)), "Timed out waiting for queue drain.");
    }
}

} // namespace TinyMemoryPool::Detail

int main()
{
    constexpr std::size_t initialCount = 4096;
    constexpr std::size_t chunkSize = 64;
    TinyMemoryPool::MemoryManager::GetInstance().Initialize(TinyMemoryPool::MemoryManagerConfig{});
    TinyMemoryPool::Detail::Pool pool;
    pool.Initialize(chunkSize, initialCount * chunkSize);

    std::vector<void*> held;
    held.reserve(initialCount * 3);
    for(std::size_t i = 0; i < initialCount; ++i)
        held.push_back(pool.Pop());
    Require(growthCount == 1, "Initialization lost a chunk or expanded early.");

    void* reserved = nullptr;
    std::thread expandingCaller([&]
    {
        pauseGrowth = true;
        reserved = pool.Pop();
    });

    Require(reachedReturn.try_acquire_for(std::chrono::seconds(10)), "Expansion did not reach return.");
    Require(grownChunks == initialCount * 2, "Unexpected growth size.");

    // 확장 호출자가 락을 보유한 채 멈춘 동안, 큐에 공개된 청크를 모두 가져간다.
    for(std::size_t i = 1; i < grownChunks; ++i)
        held.push_back(pool.Pop());
    Require(growthCount == 2, "Draining published chunks triggered another expansion.");
    resumeReturn.release();
    expandingCaller.join();
    Require(reserved != nullptr, "Expansion did not return its reserved chunk.");
    held.push_back(reserved);

    std::unordered_set<void*> unique(held.begin(), held.end());
    Require(unique.size() == initialCount * 3, "Reserved chunk was published or a live chunk was duplicated.");
    for(auto* ptr : held)
        pool.Push(ptr);

    for(auto& ptr : held)
        ptr = pool.Pop();
    Require(growthCount == 2, "Returned chunks were not reused.");
    unique = std::unordered_set<void*>(held.begin(), held.end());
    Require(unique.size() == held.size(), "Reused chunks are not unique.");
    for(auto* ptr : held)
        pool.Push(ptr);
    pool.Shutdown();
}
