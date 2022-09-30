#pragma once

#include <string>
#include <string_view>

#include <core/stl/memory.h>

namespace luisa {

using string = std::basic_string<char, std::char_traits<char>, allocator<char>>;
using std::string_view;

}// namespace luisa
