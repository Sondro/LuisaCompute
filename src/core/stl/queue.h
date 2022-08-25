#pragma once
#include <core/stl.h>
#include <EASTL/deque.h>
#include <EASTL/queue.h>
namespace luisa {
template<typename T, typename Alloc = allocator<T>>
using deque = eastl::deque<T, Alloc>;
template<typename T, typename Container = luisa::deque<T>>
using queue = eastl::queue<T, Container>;
}