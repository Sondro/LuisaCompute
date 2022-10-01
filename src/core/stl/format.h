//
// Created by Mike on 2022/9/30.
//

#pragma once

#include <fmt/format.h>
#include <core/stl/string.h>

namespace luisa {

#ifndef FMT_STRING
#define FMT_STRING(...) __VA_ARGS__
#endif

template<typename FMT, typename... Args>
[[nodiscard]] inline auto format(FMT &&f, Args &&...args) noexcept {
    using memory_buffer = fmt::basic_memory_buffer<char, fmt::inline_buffer_size, luisa::allocator<char>>;
    memory_buffer buffer;
    fmt::format_to(std::back_inserter(buffer), std::forward<FMT>(f), std::forward<Args>(args)...);
    return luisa::string{buffer.data(), buffer.size()};
}

[[nodiscard]] inline auto hash_to_string(uint64_t hash) noexcept {
    return luisa::format("{:016X}", hash);
}

}// namespace luisa