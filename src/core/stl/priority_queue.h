#pragma once
#include <core/dll_export.h>
#include <EASTL/priority_queue.h>
namespace luisa {
template<typename T, typename Container = eastl::vector<T>, typename Compare = eastl::less<typename Container::value_type>>
using priority_queue = eastl::priority_queue<T, Container, Compare>;
}