#pragma once
#include <stdint.h>
namespace luisa::compute {
enum class StreamTag : uint32_t {
    GRAPHICS,
    COMPUTE,
    COPY
};
};