#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include <core/stl/memory.h>
#include <core/stl/functional.h>
#include <core/stl/hash.h>

namespace luisa {

template<typename K, typename V,
         typename Hash = hash<K>,
         typename Eq = equal_to<>,
         typename Allocator = luisa::allocator<std::pair<const K, V>>>
using unordered_map = absl::flat_hash_map<K, V, Hash, Eq, Allocator>;

template<typename K,
         typename Hash = hash<K>,
         typename Eq = equal_to<>,
         typename Allocator = luisa::allocator<const K>>
using unordered_set = absl::flat_hash_set<K, Hash, Eq, Allocator>;

}// namespace luisa
