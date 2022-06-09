#pragma once
#include <DXRuntime/Device.h>
#include <Resource/DefaultBuffer.h>
#include <runtime/command.h>
namespace toolhub::directx {
class CommandBufferBuilder;
class ResourceStateTracker;
class TopAccel;
struct BottomAccelData {
    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomStruct;
};
struct BottomBuffer {
    DefaultBuffer defaultBuffer;
    uint64 compactSize;
    template<typename... Args>
        requires(std::is_constructible_v<DefaultBuffer, Args &&...>)
    BottomBuffer(Args &&...args)
        : defaultBuffer(std::forward<Args>(args)...) {}
};
class BottomAccel : public vstd::IOperatorNewBase {
    friend class TopAccel;
    vstd::unique_ptr<DefaultBuffer> accelBuffer;
    uint64 compactSize;
    Device *device;
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS hint;
    bool update = false;
    void SyncTopAccel() const;

public:
    bool RequireCompact() const;
    Buffer const *GetAccelBuffer() const {
        return accelBuffer.get();
    }
    BottomAccel(
        Device *device,

        luisa::compute::AccelUsageHint hint,
        bool allow_compact, bool allow_update);
    size_t PreProcessStates(
        CommandBufferBuilder &builder,
        ResourceStateTracker &tracker,
        bool update,
        Buffer const *vHandle,
        size_t vertStride, size_t vertOffset, size_t vertCount,
        Buffer const *iHandle,
        size_t idxOffset, size_t idxSize,
        BottomAccelData &bottomData);
    void UpdateStates(
        CommandBufferBuilder &builder,
        BufferView const &scratchBuffer,
        BottomAccelData &accelData);
    bool CheckAccel(
        CommandBufferBuilder &builder);
    void FinalCopy(
        CommandBufferBuilder &builder,
        BufferView const &scratchBuffer);
    ~BottomAccel();
};
}// namespace toolhub::directx