#pragma once
#include <vstl/Common.h>
#include <runtime/device.h>
#include <DXRuntime/Device.h>
#include <DXRuntime/CommandQueue.h>
#include <DXRuntime/CommandAllocator.h>
#include <DXRuntime/CommandBuffer.h>
#include <runtime/command_list.h>
#include <DXRuntime/ResourceStateTracker.h>
#include <backends/common/command_reorder_visitor.h>
using namespace luisa::compute;
namespace toolhub::directx {
class RenderTexture;
class LCSwapChain;
class BottomAccel;
struct ButtomCompactCmd {
    vstd::variant<BottomAccel *, TopAccel *> accel;
    size_t offset;
    size_t size;
};
class LCCmdBuffer final : public vstd::IOperatorNewBase {
protected:
    Device *device;
    ResourceStateTracker tracker;
    uint64 lastFence = 0;
    CommandReorderVisitor reorder;

public:
    CommandQueue queue;
    LCCmdBuffer(
        Device *device,
        IGpuAllocator *resourceAllocator,
        D3D12_COMMAND_LIST_TYPE type);
    void Execute(
        CommandList &&cmdList,
        size_t maxAlloc = std::numeric_limits<size_t>::max());
    void Sync();
    void Present(
        LCSwapChain *swapchain,
        RenderTexture *rt,
        size_t maxAlloc = std::numeric_limits<size_t>::max());
    void CompressBC(
        RenderTexture *rt,
        luisa::vector<std::byte> &result,
        bool isHDR,
        float alphaImportance,
        IGpuAllocator* allocator,
        size_t maxAlloc = std::numeric_limits<size_t>::max());
};

}// namespace toolhub::directx