//
// Created by Mike Smith on 2021/7/30.
//

#pragma once
#include <core/dll_export.h>
#include <core/stl/memory.h>

namespace luisa::compute {
class DeviceInterface;
class LC_RUNTIME_API Resource {

public:
    enum struct Tag : uint32_t {
        BUFFER,
        TEXTURE,
        BINDLESS_ARRAY,
        MESH,
        ACCEL,
        STREAM,
        EVENT,
        SHADER,
        RASTER_SHADER,
        SWAP_CHAIN,
        DEPTH_BUFFER
    };

private:
    luisa::shared_ptr<DeviceInterface> _device{nullptr};
    uint64_t _handle{0u};
    Tag _tag{};

protected:
    void _destroy() noexcept;

public:
    Resource() noexcept = default;
    Resource(DeviceInterface *device, Tag tag, uint64_t handle) noexcept;
    virtual ~Resource() noexcept { _destroy(); }
    Resource(Resource &&) noexcept;
    Resource(const Resource &) noexcept = delete;
    Resource &operator=(Resource &&) noexcept;
    Resource &operator=(const Resource &) noexcept = delete;
    [[nodiscard]] auto device() const noexcept { return _device.get(); }
    [[nodiscard]] auto handle() const noexcept { return _handle; }
    [[nodiscard]] void *native_handle() const noexcept;
    [[nodiscard]] auto tag() const noexcept { return _tag; }
    [[nodiscard]] explicit operator bool() const noexcept { return _device != nullptr; }
};

}// namespace luisa::compute
