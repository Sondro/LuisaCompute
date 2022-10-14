#pragma once
#include <d3dx12.h>
namespace toolhub::directx {
class CommandBufferBuilder;
class Resource;
class ResourceStateTracker : public vstd::IOperatorNewBase {
private:
    struct State {
        uint64 fence;
        D3D12_RESOURCE_STATES lastState;
        D3D12_RESOURCE_STATES curState;
        bool uavBarrier;
        bool isWrite;
    };
    vstd::HashMap<Resource const *, State> stateMap;
    vstd::HashMap<Resource const *> writeStateMap;
    vstd::vector<D3D12_RESOURCE_BARRIER> states;
    void ExecuteStateMap();
    void RestoreStateMap();
    void MarkWritable(Resource const *res, bool writable);
    uint64 fenceCount = 1;

public:
    D3D12_COMMAND_LIST_TYPE listType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    D3D12_RESOURCE_STATES BufferReadState() const {
        switch (listType) {
            case D3D12_COMMAND_LIST_TYPE_COMPUTE:
                return (D3D12_RESOURCE_STATES)(0x1 | 0x40 | 0x800);
            case D3D12_COMMAND_LIST_TYPE_COPY:
                return D3D12_RESOURCE_STATE_COPY_SOURCE;
            default:
                return (D3D12_RESOURCE_STATES)(0x1 | 0x2 | 0x80 | 0x40 | 0x800);
        }
    }
    D3D12_RESOURCE_STATES TextureReadState() const {
        switch (listType) {
            case D3D12_COMMAND_LIST_TYPE_COMPUTE:
                return (D3D12_RESOURCE_STATES)(0x40 | 0x800);
            case D3D12_COMMAND_LIST_TYPE_COPY:
                return D3D12_RESOURCE_STATE_COPY_SOURCE;
            default:
                return (D3D12_RESOURCE_STATES)(0x80 | 0x40 | 0x800);
        }
    }

    void ClearFence() { fenceCount++; }
    vstd::HashMap<Resource const *> const &WriteStateMap() const { return writeStateMap; }
    ResourceStateTracker();
    ~ResourceStateTracker();
    void RecordState(
        Resource const *resource,
        D3D12_RESOURCE_STATES state,
        bool lock = false);
    void RecordState(Resource const *resource, bool lock = false);
    void UpdateState(CommandBufferBuilder const &cmdBuffer);
    void RestoreState(CommandBufferBuilder const &cmdBuffer);
};
}// namespace toolhub::directx