#pragma once

#include <EASTL/list.h>
#include <EASTL/slist.h>
#include <core/stl/memory.h>

namespace luisa {

template<typename T, typename Alloc = allocator<T>>
using forward_list = eastl::slist<T, Alloc>;

template<typename T, typename Alloc = allocator<T>>
using list = eastl::list<T, Alloc>;

}// namespace luisa
