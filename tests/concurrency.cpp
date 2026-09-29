#include <TinyMemoryPool/Allocator.h>

#include <array>
#include <barrier>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <unordered_set>

namespace
{

void Require(bool condition, const char* message)
{
    if(!condition)
    {
        std::cerr << message << '\n';
        std::abort();
    }
}

} // namespace

int main()
{
    constexpr std::size_t threadCount = 8;
    constexpr std::size_t allocationsPerThread = 4096;
    constexpr std::size_t rounds = 3;
    std::array<std::array<std::uint64_t*, allocationsPerThread>, threadCount> pointers{};
    std::array<std::thread, threadCount> workers;
    std::barrier gate(static_cast<std::ptrdiff_t>(threadCount));

    for(std::size_t worker = 0; worker < threadCount; ++worker)
    {
        workers[worker] = std::thread([&, worker]
        {
            TinyMemoryPool::Allocator<std::uint64_t> allocator;
            for(std::size_t round = 0; round < rounds; ++round)
            {
                // 첫 반복에서는 여러 스레드가 동시에 공유 관리자를 처음 사용한다.
                gate.arrive_and_wait();
                for(std::size_t i = 0; i < allocationsPerThread; ++i)
                {
                    auto* ptr = allocator.allocate(1);
                    *ptr = round * threadCount * allocationsPerThread + worker * allocationsPerThread + i;
                    pointers[worker][i] = ptr;
                }
                gate.arrive_and_wait();

                if(worker == 0)
                {
                    std::unordered_set<std::uint64_t*> livePointers;
                    livePointers.reserve(threadCount * allocationsPerThread);
                    for(std::size_t owner = 0; owner < threadCount; ++owner)
                    {
                        for(std::size_t i = 0; i < allocationsPerThread; ++i)
                        {
                            auto* ptr = pointers[owner][i];
                            Require(livePointers.insert(ptr).second, "Duplicate live allocation.");
                            Require(*ptr == round * threadCount * allocationsPerThread + owner * allocationsPerThread + i,
                                    "Allocation contents changed.");
                        }
                    }
                }
                gate.arrive_and_wait();

                // 모든 읽기가 끝난 뒤, 할당한 스레드와 다른 스레드가 메모리를 반납한다.
                const std::size_t owner = (worker + 1) % threadCount;
                for(auto* ptr : pointers[owner])
                    allocator.deallocate(ptr, 1);
                gate.arrive_and_wait();
            }
        });
    }

    for(auto& worker : workers)
        worker.join();
}
