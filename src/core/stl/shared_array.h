#pragma once
#include <EASTL/shared_array.h>
namespace luisa {
template<typename T, typename Alloc = allocator<T>>
using shared_array = eastl::shared_array<T, Alloc>;
}