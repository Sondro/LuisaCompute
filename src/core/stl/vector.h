#pragma once
#include <core/stl.h>
#include <EASTL/vector.h>
namespace luisa {
template<typename T, typename Alloc = allocator<T>>
using vector = eastl::vector<T, Alloc>;
}