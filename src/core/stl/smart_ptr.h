#pragma once
#include <core/stl.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/shared_ptr.h>
namespace luisa {
using eastl::const_pointer_cast;
using eastl::dynamic_pointer_cast;
using eastl::enable_shared_from_this;
using eastl::make_shared;
using eastl::make_unique;
using eastl::reinterpret_pointer_cast;
using eastl::shared_ptr;
using eastl::static_pointer_cast;
using eastl::unique_ptr;
using eastl::weak_ptr;
}// namespace luisa