//
// Created by Mike Smith on 2021/7/22.
//

#import <backends/metal/metal_stream.h>
#import <backends/metal/metal_mesh.h>

namespace luisa::compute::metal {

MetalMesh::MetalMesh(AccelBuildHint build_hint, AccelUpdateHint update_hint) noexcept
    : _build_hint{build_hint}, _update_hint{update_hint} {}

id<MTLCommandBuffer> MetalMesh::build(MetalStream *stream, id<MTLCommandBuffer> command_buffer, const MeshBuildCommand &cmd) noexcept {

    if (cmd.triangle_buffer_offset() != 0u ||
        cmd.vertex_buffer_offset() != 0u) [[unlikely]] {
        LUISA_WARNING_WITH_LOCATION(
            "Metal seems to have trouble with non-zero-offset "
            "vertex and/or index buffers in mesh.");
    }
    auto requires_new_descriptor = [this, &cmd] {
        if (_descriptor == nullptr) { return true; }
        auto geom_desc = static_cast<const MTLAccelerationStructureTriangleGeometryDescriptor *>(
            _descriptor.geometryDescriptors[0]);
        return geom_desc.vertexBuffer == (__bridge id<MTLBuffer>)(reinterpret_cast<void *>(cmd.vertex_buffer())) &&
               geom_desc.vertexBufferOffset == cmd.vertex_buffer_offset() &&
               geom_desc.vertexStride == cmd.vertex_stride() &&
               geom_desc.indexBuffer == (__bridge id<MTLBuffer>)(reinterpret_cast<void *>(cmd.triangle_buffer())) &&
               geom_desc.indexBufferOffset == cmd.triangle_buffer_offset() &&
               geom_desc.triangleCount == cmd.triangle_buffer_size() / sizeof(Triangle);
    }();
    if (requires_new_descriptor) {
        auto mesh_desc = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];
        mesh_desc.vertexBuffer = (__bridge id<MTLBuffer>)(reinterpret_cast<void *>(cmd.vertex_buffer()));
        mesh_desc.vertexBufferOffset = cmd.vertex_buffer_offset();
        mesh_desc.vertexStride = cmd.vertex_stride();
        mesh_desc.indexBuffer = (__bridge id<MTLBuffer>)(reinterpret_cast<void *>(cmd.triangle_buffer()));
        mesh_desc.indexBufferOffset = cmd.triangle_buffer_offset();
        mesh_desc.indexType = MTLIndexTypeUInt32;
        mesh_desc.triangleCount = cmd.triangle_buffer_size() / sizeof(Triangle);
        mesh_desc.opaque = YES;
        mesh_desc.allowDuplicateIntersectionFunctionInvocation = NO;
        _descriptor = [MTLPrimitiveAccelerationStructureDescriptor descriptor];
        _descriptor.geometryDescriptors = @[mesh_desc];
        if (_update_hint == AccelUpdateHint::ALLOW_REFIT) {
            if (_build_hint == AccelBuildHint::FAST_TRACE ||
                _build_hint == AccelBuildHint::FAST_TRACE_NO_COMPACTION) {
                _descriptor.usage = MTLAccelerationStructureUsageRefit;
            } else {
                _descriptor.usage = MTLAccelerationStructureUsageRefit |
                                    MTLAccelerationStructureUsagePreferFastBuild;
            }
        } else {
            if (_build_hint == AccelBuildHint::FAST_TRACE ||
                _build_hint == AccelBuildHint::FAST_TRACE_NO_COMPACTION) {
                _descriptor.usage = MTLAccelerationStructureUsageNone;
            } else {
                _descriptor.usage = MTLAccelerationStructureUsagePreferFastBuild;
            }
        }
    }

    // update only
    auto device = command_buffer.device;
    if (_update_hint == AccelUpdateHint::ALLOW_REFIT &&
        cmd.request() == AccelBuildRequest::PREFER_UPDATE &&
        !requires_new_descriptor && _handle != nullptr) {
        if (_update_buffer == nullptr || _update_buffer.length < _update_buffer_size) {
            _update_buffer = [device newBufferWithLength:_update_buffer_size
                                                 options:MTLResourceStorageModePrivate];
        }
        auto command_encoder = [command_buffer accelerationStructureCommandEncoder];
        [command_encoder refitAccelerationStructure:_handle
                                         descriptor:_descriptor
                                        destination:_handle
                                      scratchBuffer:_update_buffer
                                scratchBufferOffset:0u];
        [command_encoder endEncoding];
        return command_buffer;
    }

    // force build (or rebuild)
    auto sizes = [device accelerationStructureSizesWithDescriptor:_descriptor];
    _update_buffer_size = sizes.refitScratchBufferSize;
    _handle = [device newAccelerationStructureWithSize:sizes.accelerationStructureSize];
    auto scratch_buffer = [device newBufferWithLength:sizes.buildScratchBufferSize
                                              options:MTLResourceStorageModePrivate];
    auto command_encoder = [command_buffer accelerationStructureCommandEncoder];
    [command_encoder buildAccelerationStructure:_handle
                                     descriptor:_descriptor
                                  scratchBuffer:scratch_buffer
                            scratchBufferOffset:0u];
    // TODO: do we need this?
    [command_encoder useResource:vertex_buffer() usage:MTLResourceUsageRead];
    [command_encoder useResource:triangle_buffer() usage:MTLResourceUsageRead];
    [command_encoder endEncoding];

    if (_build_hint == AccelBuildHint::FAST_TRACE) {
        auto pool = &stream->download_host_buffer_pool();
        auto compacted_size_buffer = pool->allocate(sizeof(uint));
        command_encoder = [command_buffer accelerationStructureCommandEncoder];
        [command_encoder writeCompactedAccelerationStructureSize:_handle
                                                        toBuffer:compacted_size_buffer.handle()
                                                          offset:compacted_size_buffer.offset()];
        // TODO: do we need this?
        [command_encoder useResource:vertex_buffer() usage:MTLResourceUsageRead];
        [command_encoder useResource:triangle_buffer() usage:MTLResourceUsageRead];
        [command_encoder endEncoding];
        stream->dispatch(command_buffer);
        [command_buffer waitUntilCompleted];
        auto compacted_size = *reinterpret_cast<const uint *>(
            static_cast<const std::byte *>(compacted_size_buffer.handle().contents) +
            compacted_size_buffer.offset());
        pool->recycle(compacted_size_buffer);
        command_buffer = stream->command_buffer();
        if (compacted_size < sizes.accelerationStructureSize) {
            LUISA_INFO("Compacting mesh from {} bytes to {} bytes.",
                       sizes.accelerationStructureSize, compacted_size);
            auto accel_before_compaction = _handle;
            _handle = [device newAccelerationStructureWithSize:compacted_size];
            command_encoder = [command_buffer accelerationStructureCommandEncoder];
            [command_encoder copyAndCompactAccelerationStructure:accel_before_compaction
                                         toAccelerationStructure:_handle];
            // TODO: do we need this?
            [command_encoder useResource:vertex_buffer() usage:MTLResourceUsageRead];
            [command_encoder useResource:triangle_buffer() usage:MTLResourceUsageRead];
            [command_encoder endEncoding];
        }
    }
    return command_buffer;
}

id<MTLBuffer> MetalMesh::vertex_buffer() const noexcept {
    LUISA_ASSERT(_descriptor != nullptr, "Mesh not built.");
    auto geom_desc = static_cast<const MTLAccelerationStructureTriangleGeometryDescriptor *>(
        _descriptor.geometryDescriptors[0]);
    return geom_desc.vertexBuffer;
}

id<MTLBuffer> MetalMesh::triangle_buffer() const noexcept {
    LUISA_ASSERT(_descriptor != nullptr, "Mesh not built.");
    auto geom_desc = static_cast<const MTLAccelerationStructureTriangleGeometryDescriptor *>(
        _descriptor.geometryDescriptors[0]);
    return geom_desc.indexBuffer;
}

}// namespace luisa::compute::metal
