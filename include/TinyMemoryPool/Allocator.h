#pragma once

#include "Detail/MemoryApi.h"

#include <cstddef>
#include <limits>
#include <new>
#include <type_traits>

namespace TinyMemoryPool
{

/// @brief STL 호환 커스텀 Allocator.
// 내부적으로 PoolManager를 통해 메모리를 할당/해제한다.
template <typename T>
class Allocator
{
  public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using is_always_equal = std::true_type;

    Allocator() noexcept = default;
    Allocator(const Allocator&) noexcept = default;

    template <typename U>
    Allocator(const Allocator<U>&) noexcept
    {
    }

    ~Allocator() noexcept = default;

    [[nodiscard]] T* allocate(std::size_t n)
    {
        if(n > std::numeric_limits<std::size_t>::max() / sizeof(T))
        {
            throw std::bad_array_new_length();
        }

        void* ptr = Detail::EngineAllocate(n * sizeof(T));

        if(ptr == nullptr) [[unlikely]]
        {
            throw std::bad_alloc();
        }

        return static_cast<T*>(ptr);
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
        Detail::EngineDeallocate(p, n * sizeof(T));
    }

    template <typename U>
    struct rebind
    {
        using other = Allocator<U>;
    };
};

template <typename T, typename U>
bool operator==(const Allocator<T>&, const Allocator<U>&) noexcept
{
    return true;
}

template <typename T, typename U>
bool operator!=(const Allocator<T>&, const Allocator<U>&) noexcept
{
    return false;
}

} // namespace TinyMemoryPool
