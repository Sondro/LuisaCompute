
#include <Shader/Shader.h>
#include <d3dcompiler.h>
#include <DXRuntime/CommandBuffer.h>
#include <DXRuntime/GlobalSamplers.h>
#include <Resource/TopAccel.h>
#include <Resource/DefaultBuffer.h>
#include <Shader/ShaderSerializer.h>

namespace toolhub::directx {
Shader::Shader(
    vstd::span<Property const> prop,
    ComPtr<ID3D12RootSignature> &&rootSig)
    : rootSig(std::move(rootSig)) {
    properties.push_back_all(prop);
}

Shader::Shader(
    vstd::span<Property const> prop,
    ID3D12Device *device) {
    properties.push_back_all(prop);
    auto serializedRootSig = ShaderSerializer::SerializeRootSig(
        prop);
    ThrowIfFailed(device->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(rootSig.GetAddressOf())));
}

void Shader::SetComputeResource(
    uint propertyName,
    CommandBufferBuilder *cb,
    BufferView buffer) const {
    auto cmdList = cb->CmdList();
    auto &&var = properties[propertyName];
    switch (var.type) {
        case ShaderVariableType::ConstantBuffer: {
            cmdList->SetComputeRootConstantBufferView(
                propertyName,
                buffer.buffer->GetAddress() + buffer.offset);
        } break;
        case ShaderVariableType::StructuredBuffer: {
            cmdList->SetComputeRootShaderResourceView(
                propertyName,
                buffer.buffer->GetAddress() + buffer.offset);
        } break;
        case ShaderVariableType::RWStructuredBuffer: {
            cmdList->SetComputeRootUnorderedAccessView(
                propertyName,
                buffer.buffer->GetAddress() + buffer.offset);
        } break;
    }
}
void Shader::SetComputeResource(
    uint propertyName,
    CommandBufferBuilder *cb,
    DescriptorHeapView view) const {
    auto cmdList = cb->CmdList();
    auto &&var = properties[propertyName];
    switch (var.type) {
        case ShaderVariableType::UAVDescriptorHeap:
        case ShaderVariableType::CBVDescriptorHeap:
        case ShaderVariableType::SampDescriptorHeap:
        case ShaderVariableType::SRVDescriptorHeap: {
            cmdList->SetComputeRootDescriptorTable(
                propertyName,
                view.heap->hGPU(view.index));
        } break;
    }
}
void Shader::SetComputeResource(
    uint propertyName,
    CommandBufferBuilder *cmdList,
    TopAccel const *bAccel) const {
    return SetComputeResource(
        propertyName,
        cmdList,
        BufferView(bAccel->GetAccelBuffer()));
}
}// namespace toolhub::directx