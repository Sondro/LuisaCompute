#pragma once
#include <vstl/Common.h>
#include <Windows.h>
#include <d3dx12.h>
#include <Shader/ShaderVariableType.h>
#include <Resource/Buffer.h>
#include <Resource/DescriptorHeap.h>
namespace toolhub::directx {
class TopAccel;
class CommandBufferBuilder;
class Shader : public vstd::IOperatorNewBase {
public:
    enum class Tag : vbyte {
        ComputeShader,
        RayTracingShader
    };
    virtual Tag GetTag() const = 0;

protected:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
    vstd::vector<Property> properties;
    uint bindlessCount;

public:
    uint BindlessCount() const { return bindlessCount; }

    Shader(
        vstd::span<Property const> properties,
        ID3D12Device *device);
    Shader(
        vstd::span<Property const> properties,
        ComPtr<ID3D12RootSignature> &&rootSig);
    ID3D12RootSignature *RootSig() const { return rootSig.Get(); }

    void SetComputeResource(
        uint propertyName,
        CommandBufferBuilder *cmdList,
        BufferView buffer) const;
    void SetComputeResource(
        uint propertyName,
        CommandBufferBuilder *cmdList,
        DescriptorHeapView view) const;
    void SetComputeResource(
        uint propertyName,
        CommandBufferBuilder *cmdList,
        TopAccel const *bAccel) const;

    KILL_COPY_CONSTRUCT(Shader)
    KILL_MOVE_CONSTRUCT(Shader)
};
}// namespace toolhub::directx