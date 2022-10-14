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
struct ReorderFuncTable {
    bool is_res_in_bindless(uint64_t bindless_handle, uint64_t resource_handle) const noexcept;
    Usage get_usage(uint64_t shader_handle, size_t argument_index) const noexcept;
};
class LCCmdBuffer final : public vstd::IOperatorNewBase {
protected:
    Device *device;
    ResourceStateTracker tracker;
    uint64 lastFence = 0;
    ReorderFuncTable reorderFuncTable;
    CommandReorderVisitor<ReorderFuncTable, false> reorder;

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
        TextureBase *rt,
        size_t maxAlloc = std::numeric_limits<size_t>::max());
    void CompressBC(
        TextureBase *rt,
        luisa::vector<std::byte> &result,
        bool isHDR,
        float alphaImportance,
        IGpuAllocator *allocator,
        size_t maxAlloc = std::numeric_limits<size_t>::max());
};

}// namespace toolhub::directx