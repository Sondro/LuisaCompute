#pragma once
#include <core/stl.h>
#include <EASTL/variant.h>
namespace luisa {
using eastl::monostate;
using eastl::variant;
using eastl::variant_alternative_t;
using eastl::variant_size_v;
using eastl::get_if;
using eastl::holds_alternative;
using eastl::visit;
using eastl::get;
}