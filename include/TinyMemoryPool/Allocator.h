#pragma once

#include <cstddef>
#include <limits>
#include <new>         // std::bad_alloc, std::bad_array_new_length
#include <type_traits> // std::true_type

#include "Detail/MemoryApi.h"

namespace TinyMemoryPool
{

template <typename T> class Allocator
{
  public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using is_always_equal = std::true_type;

    Allocator() noexcept = default;
    Allocator(const Allocator&) noexcept = default;

    template <typename U> Allocator(const Allocator<U>&) noexcept
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

    template <typename U> struct rebind
    {
        using other = Allocator<U>;
    };
};

template <typename T, typename U> bool operator==(const Allocator<T>&, const Allocator<U>&) noexcept
{
    return true;
}

template <typename T, typename U> bool operator!=(const Allocator<T>&, const Allocator<U>&) noexcept
{
    return false;
}

} // namespace TinyMemoryPool