#include <TinyMemoryPool/Allocator.h>
#include "../examples/PoolContainers.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace
{

void Require(bool condition, const char* message)
{
    if(!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

template <std::size_t Alignment>
struct alignas(Alignment) AlignedValue
{
    std::array<std::uint64_t, 8> Values{};
};

template <typename T>
void CheckAlignment(std::size_t count)
{
    TinyMemoryPool::Allocator<T> allocator;
    T* data = allocator.allocate(count);
    if(reinterpret_cast<std::uintptr_t>(data) % alignof(T) != 0)
    {
        allocator.deallocate(data, count);
        Require(false, "Allocation did not satisfy alignof(T).");
    }

    for(std::size_t i = 0; i < count; ++i)
    {
        std::construct_at(data + i);
        data[i].Values[0] = i + 1;
    }
    for(std::size_t i = 0; i < count; ++i)
        Require(data[i].Values[0] == i + 1, "Allocated objects overlapped.");
    std::destroy_n(data, count);
    allocator.deallocate(data, count);
}

template <std::size_t Alignment>
void CheckAlignmentSizes()
{
    // 작은 할당, 배열, 풀 상한을 넘는 할당에서 정렬을 확인한다.
    for(const std::size_t count : {1u, 7u, 257u})
        CheckAlignment<AlignedValue<Alignment>>(count);
}

void TestFundamentalAlignment()
{
    CheckAlignmentSizes<alignof(std::max_align_t)>();
    CheckAlignmentSizes<16>();
}

void TestExtendedAlignment()
{
    CheckAlignmentSizes<32>();
    CheckAlignmentSizes<64>();
    CheckAlignmentSizes<256>();
    CheckAlignmentSizes<4096>();
}

void TestSizeBoundaries()
{
    TinyMemoryPool::Allocator<std::byte> allocator;
    constexpr auto alignment = TinyMemoryPool::Detail::PoolAlignment;
    const std::array<std::size_t, 21> sizes{
        0, 1, 47, 48, 49, 111, 112, 113, 240, 241, 496,
        497, 1008, 1009, 2032, 2033, 4080, 4081, 4096, 8192, 16384};
    for(const auto size : sizes)
    {
        auto* data = allocator.allocate(size);
        Require(reinterpret_cast<std::uintptr_t>(data) % alignment == 0,
                "The byte allocation path lost the pool alignment.");
        for(std::size_t i = 0; i < size; ++i)
            std::construct_at(data + i, static_cast<std::byte>(i % 251));
        for(std::size_t i = 0; i < size; ++i)
            Require(data[i] == static_cast<std::byte>(i % 251), "Size boundary write failed.");
        allocator.deallocate(data, size);
    }
}

void TestReuse()
{
    TinyMemoryPool::Allocator<std::uint64_t> allocator;
    std::vector<std::uint64_t*> blocks;
    for(std::size_t i = 0; i < 5000; ++i)
    {
        auto* block = allocator.allocate(1);
        std::construct_at(block, i);
        blocks.push_back(block);
    }
    for(std::size_t i = 0; i < blocks.size(); ++i)
        Require(*blocks[i] == i, "Live pool blocks overlapped during growth.");
    for(auto* block : blocks)
        allocator.deallocate(block, 1);
    for(std::size_t i = 0; i < 5000; ++i)
    {
        auto* block = allocator.allocate(1);
        std::construct_at(block, i);
        Require(*block == i, "Pool reuse failed.");
        allocator.deallocate(block, 1);
    }
}

void TestContainers()
{
    using Value = AlignedValue<64>;
    std::vector<Value, TinyMemoryPool::Allocator<Value>> values;
    for(std::size_t i = 0; i < 300; ++i)
    {
        values.emplace_back();
        values.back().Values[0] = i;
    }
    Require(reinterpret_cast<std::uintptr_t>(values.data()) % alignof(Value) == 0,
            "Vector data is misaligned.");
    for(std::size_t i = 0; i < values.size(); ++i)
        Require(values[i].Values[0] == i, "Vector reallocation lost data.");

    using Entry = std::pair<const int, Value>;
    std::map<int, Value, std::less<int>, TinyMemoryPool::Allocator<Entry>> nodes;
    for(int i = 0; i < 100; ++i)
        nodes[i].Values[0] = static_cast<std::uint64_t>(i);
    for(const auto& [key, value] : nodes)
    {
        Require(reinterpret_cast<std::uintptr_t>(&value) % alignof(Value) == 0,
                "Rebound node allocator returned misaligned storage.");
        Require(value.Values[0] == static_cast<std::uint64_t>(key), "Map value mismatch.");
    }
    auto copy = nodes;
    nodes.clear();
    Require(copy.size() == 100, "Container copy did not retain nodes.");

    // 사용 예제는 짧게 유지하고, 별칭의 확장·복사·이동 검증은 여기서 수행한다.
    using namespace TinyMemoryPool::Examples;
    TVector<int> objectIds;
    for(int id = 0; id < 1000; ++id)
        objectIds.push_back(id);
    TString name = "A scene object with a name stored outside the string's inline buffer";
    THashMap<int, TString> names;
    names.emplace(objectIds.front(), name);
    names.emplace(objectIds.back(), "Last object");
    auto copiedNames = names;
    auto movedIds = std::move(objectIds);
    Require(movedIds.size() == 1000 && copiedNames.size() == 2 && copiedNames.at(0) == name,
            "Container aliases lost data during copy or move.");
}

} // namespace

int main(int argc, char** argv)
{
    static_assert(noexcept(TinyMemoryPool::Allocator<int>{}.allocate(1)));
    static_assert(noexcept(TinyMemoryPool::Allocator<AlignedValue<64>>{}.allocate(1)));
    Require(argc == 2, "Expected one test case name.");
    const std::string_view name = argv[1];
    if(name == "fundamental") TestFundamentalAlignment();
    else if(name == "extended") TestExtendedAlignment();
    else if(name == "boundaries") TestSizeBoundaries();
    else if(name == "reuse") TestReuse();
    else if(name == "containers") TestContainers();
    else Require(false, "Unknown test case.");
    std::cout << name << ": passed\n";
    return 0;
}
