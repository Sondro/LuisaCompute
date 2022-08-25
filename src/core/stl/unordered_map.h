#pragma once
#include <core/stl.h>
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

namespace luisa {
#define LUISA_COMPUTE_USE_ABSEIL_HASH_TABLES

#ifdef LUISA_COMPUTE_USE_ABSEIL_HASH_TABLES
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
#else
using std::unordered_map;
using std::unordered_set;
#endif

}