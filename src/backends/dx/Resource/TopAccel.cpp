
#include <Resource/TopAccel.h>
#include <Resource/DefaultBuffer.h>
#include <DXRuntime/CommandAllocator.h>
#include <DXRuntime/ResourceStateTracker.h>
#include <DXRuntime/CommandBuffer.h>
#include <Resource/BottomAccel.h>
namespace toolhub::directx {

namespace detail {
void GetRayTransform(D3D12_RAYTRACING_INSTANCE_DESC &inst, float4x4 const &tr) {
    float *x[3] = {inst.Transform[0],
                   inst.Transform[1],
                   inst.Transform[2]};
    for (auto i : vstd::range(4))
        for (auto j : vstd::range(3)) {
            auto ptr = reinterpret_cast<float const *>(&tr.cols[i]);
            x[j][i] = ptr[j];
        }
}
}// namespace detail
TopAccel::TopAccel(Device *device, luisa::compute::AccelUsageHint hint,
                   bool allow_compact, bool allow_update)
    : device(device) {
    //TODO: allow_compact not supported
    allow_compact = false;
    auto GetPreset = [&] {
        switch (hint) {
            case AccelUsageHint::FAST_TRACE:
                return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
            case AccelUsageHint::FAST_BUILD:
                return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
        }
    };
    memset(&topLevelBuildDesc, 0, sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC));
    memset(&topLevelPrebuildInfo, 0, sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO));
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS &topLevelInputs = topLevelBuildDesc.Inputs;
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelInputs.Flags = GetPreset();
    if (allow_compact) {
        topLevelInputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
    }
    if (allow_update) {
        topLevelInputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
    }
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    topLevelBuildDesc.Inputs.NumDescs = 0;
}

void TopAccel::UpdateMesh(
    MeshHandle *handle) {
    auto instIndex = handle->accelIndex;
    auto &&ist = allInstance[instIndex].handle;
    assert(ist == handle);
    auto mesh = handle->mesh;
    setMap.ForceEmplace(instIndex, handle);
    requireBuild = true;
}
void TopAccel::SetMesh(BottomAccel *mesh, uint64 index) {
    auto &&inst = allInstance[index].handle;
    if (inst != nullptr) {
        if (inst->mesh == mesh) return;
        inst->mesh->RemoveAccelRef(inst);
    }
    inst = mesh->AddAccelRef(this, index);
    inst->accelIndex = index;
}
TopAccel::~TopAccel() {
    for (auto &&i : allInstance) {
        auto mesh = i.handle;
        if (mesh)
            mesh->mesh->RemoveAccelRef(mesh);
    }
}
size_t TopAccel::PreProcess(
    ResourceStateTracker &tracker,
    CommandBufferBuilder &builder,
    uint64 size,
    vstd::span<AccelBuildCommand::Modification const> const &modifications,
    bool update) {
    auto refreshUpdate = vstd::create_disposer([&] { this->update = update; });
    if ((uint)(topLevelBuildDesc.Inputs.Flags &
               D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE) == 0 ||
        topLevelBuildDesc.Inputs.NumDescs != size) update = false;
    topLevelBuildDesc.Inputs.NumDescs = size;
    allInstance.resize(size);
    setDesc.clear();
    setDesc.push_back_all(modifications);

    for (auto &&m : setDesc) {
        auto ite = setMap.Find(m.index);
        bool updateMesh = (m.flags & m.flag_mesh);
        if (ite) {
            if (!updateMesh) {
                m.mesh = ite.Value()->mesh->GetAccelBuffer()->GetAddress();
                m.flags |= m.flag_mesh;
            }
            setMap.Remove(ite);
        }
        if (updateMesh) {
            auto mesh = reinterpret_cast<BottomAccel *>(m.mesh);
            SetMesh(mesh, m.index);
            m.mesh = mesh->GetAccelBuffer()->GetAddress();
            update = false;
        }
    }
    if (requireBuild) {
        requireBuild = false;
        update = false;
    }
    if (setMap.size() != 0) {
        update = false;
        setDesc.reserve(setMap.size());
        for (auto &&i : setMap) {
            auto& mod = setDesc.emplace_back(i.first);
            mod.flags = mod.flag_mesh;
            mod.mesh = i.second->mesh->GetAccelBuffer()->GetAddress();
        }
        setMap.Clear();
    }
    struct Buffers {
        vstd::unique_ptr<DefaultBuffer> v;
        Buffers(vstd::unique_ptr<DefaultBuffer> &&a)
            : v(std::move(a)) {}
        void operator()() const {}
    };
    auto GenerateNewBuffer = [&](vstd::unique_ptr<DefaultBuffer> &oldBuffer, size_t newSize, bool needCopy, D3D12_RESOURCE_STATES state) {
        if (!oldBuffer) {
            newSize = CalcAlign(newSize, 65536);
            oldBuffer = vstd::create_unique(new DefaultBuffer(
                device,
                newSize,
                device->defaultAllocator.get(),
                state));
            return true;
        } else {
            if (newSize <= oldBuffer->GetByteSize()) return false;
            newSize = CalcAlign(newSize, 65536);
            auto newBuffer = new DefaultBuffer(
                device,
                newSize,
                device->defaultAllocator.get(),
                state);
            if (needCopy) {
                tracker.RecordState(
                    oldBuffer.get(),
                    VEngineShaderResourceState);
                tracker.RecordState(
                    newBuffer,
                    D3D12_RESOURCE_STATE_COPY_DEST);
                tracker.UpdateState(builder);
                builder.CopyBuffer(
                    oldBuffer.get(),
                    newBuffer,
                    0,
                    0,
                    oldBuffer->GetByteSize());
                tracker.RecordState(
                    oldBuffer.get());
                tracker.RecordState(
                    newBuffer);
            }
            builder.GetCB()->GetAlloc()->ExecuteAfterComplete(Buffers{std::move(oldBuffer)});
            oldBuffer = vstd::create_unique(newBuffer);
            return true;
        }
    };
    size_t instanceByteCount = size * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
    auto &&input = topLevelBuildDesc.Inputs;
    device->device->GetRaytracingAccelerationStructurePrebuildInfo(&input, &topLevelPrebuildInfo);
    if (GenerateNewBuffer(instBuffer, instanceByteCount, true, VEngineShaderResourceState)) {
        topLevelBuildDesc.Inputs.InstanceDescs = instBuffer->GetAddress();
    }
    if (GenerateNewBuffer(accelBuffer, topLevelPrebuildInfo.ResultDataMaxSizeInBytes, false, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE)) {
        update = false;
        topLevelBuildDesc.DestAccelerationStructureData = accelBuffer->GetAddress();
    }
    if (update) {
        topLevelBuildDesc.SourceAccelerationStructureData = topLevelBuildDesc.DestAccelerationStructureData;
        topLevelBuildDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    } else {
        topLevelBuildDesc.SourceAccelerationStructureData = 0;
        topLevelBuildDesc.Inputs.Flags =
            (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS)(((uint)topLevelBuildDesc.Inputs.Flags) & (~((uint)D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE)));
    }

    return (update ? topLevelPrebuildInfo.UpdateScratchDataSizeInBytes : topLevelPrebuildInfo.ScratchDataSizeInBytes) + sizeof(size_t);
}
void TopAccel::Build(
    ResourceStateTracker &tracker,
    CommandBufferBuilder &builder,
    BufferView const &scratchBuffer) {
    if (Length() == 0) return;
    auto alloc = builder.GetCB()->GetAlloc();
    // Update
    if (!setDesc.empty()) {
        tracker.RecordState(
            instBuffer.get(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        tracker.UpdateState(builder);
        auto cs = device->setAccelKernel.Get(device);
        auto setBuffer = alloc->GetTempUploadBuffer(setDesc.byte_size());
        auto cbuffer = alloc->GetTempUploadBuffer(sizeof(size_t), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        struct CBuffer {
            uint dsp;
            uint count;
        };
        CBuffer cbValue;
        cbValue.dsp = setDesc.size();
        cbValue.count = Length();
        static_cast<UploadBuffer const *>(cbuffer.buffer)
            ->CopyData(cbuffer.offset,
                       {reinterpret_cast<vbyte const *>(&cbValue), sizeof(CBuffer)});
        static_cast<UploadBuffer const *>(setBuffer.buffer)
            ->CopyData(setBuffer.offset,
                       {reinterpret_cast<vbyte const *>(setDesc.data()), setDesc.byte_size()});
        BindProperty properties[3];
        properties[0] = cbuffer;
        properties[1] = setBuffer;
        properties[2] = BufferView(instBuffer.get());
        //TODO
        builder.DispatchCompute(
            cs,
            uint3(setDesc.size(), 1, 1),
            {properties, 3});
        setDesc.clear();
        tracker.RecordState(
            instBuffer.get());
        tracker.UpdateState(builder);
    }
    topLevelBuildDesc.ScratchAccelerationStructureData = scratchBuffer.buffer->GetAddress() + scratchBuffer.offset;
    if (RequireCompact()) {
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postInfo;
        postInfo.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;
        auto compactOffset = scratchBuffer.offset + scratchBuffer.byteSize - sizeof(size_t);
        postInfo.DestBuffer = scratchBuffer.buffer->GetAddress() + compactOffset;
        builder.CmdList()->BuildRaytracingAccelerationStructure(
            &topLevelBuildDesc,
            1,
            &postInfo);
    } else {
        builder.CmdList()->BuildRaytracingAccelerationStructure(
            &topLevelBuildDesc,
            0,
            nullptr);
    }
}
void TopAccel::FinalCopy(
    CommandBufferBuilder &builder,
    BufferView const &scratchBuffer) {
    auto compactOffset = scratchBuffer.offset + scratchBuffer.byteSize - sizeof(size_t);
    auto &&alloc = builder.GetCB()->GetAlloc();
    auto readback = alloc->GetTempReadbackBuffer(sizeof(size_t));
    builder.CopyBuffer(
        scratchBuffer.buffer,
        readback.buffer,
        compactOffset,
        readback.offset,
        sizeof(size_t));
    alloc->ExecuteAfterComplete([readback, this] {
        static_cast<ReadbackBuffer const *>(readback.buffer)->CopyData(readback.offset, {(vbyte *)&compactSize, sizeof(size_t)});
    });
}
bool TopAccel::RequireCompact() const {
    return (((uint)topLevelBuildDesc.Inputs.Flags & (uint)D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION) != 0) && !update;
}
bool TopAccel::CheckAccel(
    CommandBufferBuilder &builder) {
    auto disp = vstd::create_disposer([&] { compactSize = 0; });
    if (compactSize == 0)
        return false;
    auto &&alloc = builder.GetCB()->GetAlloc();
    auto newAccelBuffer = vstd::create_unique(new DefaultBuffer(
        device,
        CalcAlign(compactSize, 65536),
        device->defaultAllocator.get(),
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE));

    builder.CmdList()->CopyRaytracingAccelerationStructure(
        newAccelBuffer->GetAddress(),
        accelBuffer->GetAddress(),
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT);
    alloc->ExecuteAfterComplete([b = std::move(accelBuffer)] {});
    accelBuffer = std::move(newAccelBuffer);
    return true;
}
}// namespace toolhub::directx