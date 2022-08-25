#pragma once
#include <core/stl.h>
#include <string>
namespace luisa {
using string = std::basic_string<char, std::char_traits<char>, allocator<char>>;
using std::string_view;
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

}