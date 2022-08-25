//
// Created by Mike Smith on 2021/9/13.
//

#pragma once

#include <cstdlib>
#include <cmath>
#include <memory>
#include <assert.h>
#include <fmt/format.h>

#include <EASTL/span.h>
#include <EASTL/memory.h>

#include <core/dll_export.h>
#include <core/hash.h>

namespace luisa {

namespace detail {
LC_CORE_API void *allocator_allocate(size_t size, size_t alignment) noexcept;
LC_CORE_API void allocator_deallocate(void *p, size_t alignment) noexcept;
LC_CORE_API void *allocator_reallocate(void *p, size_t size, size_t alignment) noexcept;
}// namespace detail

[[nodiscard]] inline auto align(size_t s, size_t a) noexcept {
    return (s + a - 1u) / a * a;
}

template<typename T = std::byte>
struct allocator {
    using value_type = T;
    constexpr allocator() noexcept = default;
    explicit constexpr allocator(const char *) noexcept {}
    template<typename U>
    constexpr allocator(allocator<U>) noexcept {}
    [[nodiscard]] auto allocate(std::size_t n) const noexcept {
        return static_cast<T *>(detail::allocator_allocate(sizeof(T) * n, alignof(T)));
    }
    [[nodiscard]] auto allocate(std::size_t n, size_t alignment, size_t) const noexcept {
        assert(alignment >= alignof(T));
        return static_cast<T *>(detail::allocator_allocate(sizeof(T) * n, alignment));
    }
    void deallocate(T *p, size_t) const noexcept {
        detail::allocator_deallocate(p, alignof(T));
    }
    void deallocate(void *p, size_t) const noexcept {
        detail::allocator_deallocate(p, alignof(T));
    }
    template<typename R>
    [[nodiscard]] constexpr auto operator==(allocator<R>) const noexcept -> bool {
        return std::is_same_v<T, R>;
    }
};

template<typename T>
[[nodiscard]] inline auto allocate(size_t n = 1u) noexcept {
    return allocator<T>{}.allocate(n);
}

template<typename T>
inline void deallocate(T *p) noexcept {
    allocator<T>{}.deallocate(p, 0u);
}

template<typename T, typename... Args>
[[nodiscard]] inline auto new_with_allocator(Args &&...args) noexcept {
    return std::construct_at(allocate<T>(), std::forward<Args>(args)...);
}

template<typename T>
inline void delete_with_allocator(T *p) noexcept {
    if (p != nullptr) {
        std::destroy_at(p);
        deallocate(p);
    }
}



using eastl::span;
template<typename T = void>
struct equal_to {
    [[nodiscard]] bool operator()(const T &lhs, const T &rhs) const noexcept { return lhs == rhs; }
};

template<>
struct equal_to<void> {
    using is_transparent = void;
    template<typename T1, typename T2>
    [[nodiscard]] bool operator()(T1 &&lhs, T2 &&rhs) const noexcept {
        return std::forward<T1>(lhs) == std::forward<T2>(rhs);
    }
};


struct default_sentinel_t {};
inline constexpr default_sentinel_t default_sentinel{};

namespace detail {

template<typename F>
class LazyConstructor {
private:
    mutable F _ctor;

public:
    explicit LazyConstructor(F _ctor) noexcept : _ctor{_ctor} {}
    [[nodiscard]] operator auto() const noexcept { return _ctor(); }
};

}// namespace detail

template<typename F>
[[nodiscard]] auto lazy_construct(F ctor) noexcept {
    return detail::LazyConstructor<F>(ctor);
}


}// namespace luisa
