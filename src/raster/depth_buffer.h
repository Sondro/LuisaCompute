#pragma once
#include <raster/depth_format.h>
#include <runtime/resource.h>
namespace luisa::compute {
class LC_RUNTIME_API DepthBuffer : public Resource {
private:
    uint2 _size{};
    DepthFormat _format{};

public:
    DepthBuffer(Device::Interface *device, DepthFormat format, uint2 size);
    [[nodiscard]] auto size() const noexcept { return _size; }
    [[nodiscard]] auto format() const noexcept { return _format; }
    [[nodiscard]] luisa::unique_ptr<Command> clear(float value) const noexcept;
};
}// namespace luisa::computep