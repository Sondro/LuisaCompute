//
// Created by Mike Smith on 2022/6/18.
//

#pragma once

#include <nlohmann/json_fwd.hpp>
#include <core/stl.h>
#include <EASTL/map.h>
#include <core/stl/vector.h>
#include <core/stl/string.h>
namespace luisa {

template<typename K, typename V,
         typename Compare = std::less<>,
         typename Alloc = luisa::allocator<std::pair<const K, V>>>
using json_map = eastl::map<K, V, Compare, Alloc>;
using json = nlohmann::basic_json<
    json_map, luisa::vector, luisa::string,
    bool, std::int64_t, std::uint64_t, double,
    luisa::allocator, ::nlohmann::adl_serializer,
    luisa::vector<std::uint8_t>>;

}// namespace luisa
