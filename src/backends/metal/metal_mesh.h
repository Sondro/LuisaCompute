//
// Created by Mike Smith on 2021/7/22.
//

#pragma once

#import <Metal/Metal.h>
#import <rtx/mesh.h>

namespace luisa::compute::metal {

class MetalStream;

class MetalMesh {

private:
    id<MTLAccelerationStructure> _handle{nullptr};
    id<MTLBuffer> _update_buffer{nullptr};
    MTLPrimitiveAccelerationStructureDescriptor *_descriptor{nullptr};
    size_t _update_buffer_size{0u};
    AccelBuildHint _build_hint;
    AccelUpdateHint _update_hint;

public:
    MetalMesh(AccelBuildHint build_hint, AccelUpdateHint update_hint) noexcept;
    [[nodiscard]] auto handle() const noexcept { return _handle; }
    [[nodiscard]] id<MTLCommandBuffer> build(
        MetalStream *stream, id<MTLCommandBuffer> command_buffer, const MeshBuildCommand &cmd) noexcept;
    [[nodiscard]] auto descriptor() const noexcept { return _descriptor; }
    [[nodiscard]] id<MTLBuffer> vertex_buffer() const noexcept;
    [[nodiscard]] id<MTLBuffer> triangle_buffer() const noexcept;
};

}// namespace luisa::compute::metal
