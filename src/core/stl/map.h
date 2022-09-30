#pragma once

#include <absl/container/btree_set.h>
#include <absl/container/btree_map.h>

#include <core/stl/memory.h>

namespace luisa {

template<typename K, typename V,
         typename Compare = std::less<>,
         typename Alloc = luisa::allocator<std::pair<const K, V>>>
using map = absl::btree_map<K, V, Compare, Alloc>;

template<typename K, typename V,
         typename Compare = std::less<>,
         typename Alloc = luisa::allocator<std::pair<const K, V>>>
using multimap = absl::btree_multimap<K, V, Compare, Alloc>;

template<typename K,
         typename Compare = std::less<>,
         typename Alloc = luisa::allocator<K>>
using multiset = absl::btree_multiset<K, Compare, Alloc>;

template<typename K,
         typename Compare = std::less<>,
         typename Alloc = luisa::allocator<K>>
using set = absl::btree_set<K, Compare, Alloc>;

}// namespace luisa
