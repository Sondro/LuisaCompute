#pragma once

#include <EASTL/vector.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/bitvector.h>

#include <core/stl/memory.h>

namespace luisa {

template<typename T, typename Alloc = allocator<T>>
using vector = eastl::vector<T, Alloc>;
template <typename T, size_t node_count, typename Alloc = allocator<T>>
using fixed_vector = eastl::fixed_vector<T, node_count, true, Alloc>;
using eastl::bitvector;

}// namespace luisa
