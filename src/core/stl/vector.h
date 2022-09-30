#pragma once

#include <EASTL/vector.h>
#include <EASTL/bitvector.h>

#include <core/stl/memory.h>

namespace luisa {

template<typename T, typename Alloc = allocator<T>>
using vector = eastl::vector<T, Alloc>;

using eastl::bitvector;

}// namespace luisa
