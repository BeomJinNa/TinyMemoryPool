#include <TinyMemoryPool/Allocator.h>
#include "MemoryManager.h"

#include <cstddef>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <new>
#include <string_view>

#if defined(_MSC_VER)
#include <crtdbg.h>
#endif

// 이 실행 파일에서만 할당 실패를 주입한다. 실제 시스템 메모리를 소진하지 않는다.
void* operator new(std::size_t, std::align_val_t, const std::nothrow_t&) noexcept
{
    return nullptr;
}

namespace
{

struct alignas(64) AlignedValue
{
    std::byte Data[64];
};

template <typename T>
void ExpectFailure(std::size_t count)
{
    TinyMemoryPool::Allocator<T> allocator;
    T* ptr = allocator.allocate(count);
    allocator.deallocate(ptr, count);
}

} // namespace

int main(int argc, char** argv)
{
    if(argc != 2)
        return 2;

    // 예외를 끈 MSVC에서는 terminate가 abort로 이어지므로 두 종료 경로를 감지한다.
#if defined(_MSC_VER)
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif
    std::signal(SIGABRT, [](int) { std::_Exit(86); });
    std::set_terminate([] { std::_Exit(86); });
    constexpr auto maximum = std::numeric_limits<std::size_t>::max();
    const std::string_view name = argv[1];
    if(name == "header_max") ExpectFailure<std::byte>(maximum);
    else if(name == "header_wrap") ExpectFailure<std::byte>(maximum - 7);
    else if(name == "header_array") ExpectFailure<std::uint64_t>(maximum / sizeof(std::uint64_t));
    else if(name == "multiply") ExpectFailure<std::uint64_t>(maximum / sizeof(std::uint64_t) + 1);
    else if(name == "aligned_multiply") ExpectFailure<AlignedValue>(maximum / sizeof(AlignedValue) + 1);
    else if(name == "system") ExpectFailure<std::byte>(8192);
    else if(name == "aligned_system") ExpectFailure<AlignedValue>(1);
    else if(name == "block_round" || name == "reserve_exhaustion")
    {
        auto& manager = TinyMemoryPool::MemoryManager::GetInstance();
        TinyMemoryPool::MemoryManagerConfig config;
        manager.Initialize(config);
        const auto size = name == "block_round" ? maximum : config.TotalReserveSize + 1;
        (void)manager.AllocateBlock(size);
    }
    else return 2;

    std::cerr << "Allocation returned instead of terminating.\n";
    return 1;
}
