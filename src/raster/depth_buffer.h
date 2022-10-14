#pragma once
#include <raster/raster_state.h>
#include <runtime/resource.h>
namespace luisa::compute {
class DepthBuffer : public Resource {
private:
    uint2 _size{};
    DepthFormat _format{};

public:
    DepthBuffer(Device::Interface *device, DepthFormat format, uint2 size)
        : _size(size), _format(format),
          Resource(
              device,
              Tag::DEPTH_BUFFER,
              device->create_depth_buffer(format, size.x, size.y)) {
    }
    [[nodiscard]] auto size() const noexcept { return _size; }
    [[nodiscard]] auto format() const noexcept { return _format; }
};
}// namespace luisa::compute