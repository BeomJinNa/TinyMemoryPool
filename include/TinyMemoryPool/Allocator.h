#pragma once

#include "Detail/MemoryApi.h"

#include <cstddef>
#include <limits>
#include <new>
#include <type_traits>

namespace TinyMemoryPool
{

/// @brief STL 컨테이너의 메모리 할당·반납을 TinyMemoryPool에 연결한다.
/// @note 풀 수명 내 할당·해제는 동시 호출할 수 있다. 컨테이너·객체 접근의 동기화는 별도다.
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

    [[nodiscard]] T* allocate(std::size_t n) noexcept
    {
        if(n > std::numeric_limits<std::size_t>::max() / sizeof(T))
        {
            Detail::AllocationFailure("Allocation size overflow.");
        }

        void* ptr = nullptr;

        // 풀의 기본 정렬보다 높은 정렬이 필요하면 정렬 지정 new/delete를 사용한다.
        if constexpr(alignof(T) > Detail::PoolAlignment)
        {
            ptr = ::operator new(n * sizeof(T), std::align_val_t{alignof(T)}, std::nothrow);
        }
        else
        {
            ptr = Detail::EngineAllocate(n * sizeof(T));
        }

        if(ptr == nullptr) [[unlikely]]
        {
            Detail::AllocationFailure("Memory allocation failed.");
        }

        return static_cast<T*>(ptr);
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
        if constexpr(alignof(T) > Detail::PoolAlignment)
        {
            ::operator delete(p, std::align_val_t{alignof(T)});
        }
        else
        {
            Detail::EngineDeallocate(p, n * sizeof(T));
        }
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
