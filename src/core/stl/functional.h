#pragma once

#include <EASTL/functional.h>

namespace luisa {

using eastl::function;
using eastl::move_only_function;

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

}// namespace luisa
