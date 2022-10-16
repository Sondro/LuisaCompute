#include <raster/depth_buffer.h>
#include <runtime/device.h>
namespace luisa::compute {
DepthBuffer Device::create_depth_buffer(DepthFormat depth_format, uint2 size) noexcept {
    return _create<DepthBuffer>(depth_format, size);
}
DepthBuffer::DepthBuffer(Device::Interface *device, DepthFormat format, uint2 size)
    : _size(size), _format(format),
      Resource(
          device,
          Tag::DEPTH_BUFFER,
          device->create_depth_buffer(format, size.x, size.y)) {
}
luisa::unique_ptr<Command> DepthBuffer::clear(float value) const noexcept {
    return luisa::make_unique<ClearDepthCommand>(handle(), value);
}
}// namespace luisa::compute