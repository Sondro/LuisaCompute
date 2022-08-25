#pragma once
#include <core/stl.h>
#include <EASTL/list.h>
#include <EASTL/slist.h>
namespace luisa {
template<typename T, typename Alloc = allocator<T>>
using slist = eastl::slist<T, Alloc>;
template<typename T, typename Alloc = allocator<T>>
using list = eastl::list<T, Alloc>;
}// namespace luisa