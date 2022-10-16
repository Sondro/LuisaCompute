
#include <Shader/Shader.h>
#include <d3dcompiler.h>
#include <DXRuntime/CommandBuffer.h>
#include <DXRuntime/GlobalSamplers.h>
#include <Resource/TopAccel.h>
#include <Resource/DefaultBuffer.h>
#include <Shader/ShaderSerializer.h>

namespace toolhub::directx {
SavedArgument::SavedArgument(Usage usage, Variable const &var)
    : SavedArgument(var.type()) {
    varUsage = usage;
}

SavedArgument::SavedArgument(Function kernel, Variable const &var)
    : SavedArgument(var.type()) {
    varUsage = kernel.variable_usage(var.uid());
}
SavedArgument::SavedArgument(Type const *type) {
    tag = type->tag();
    switch (type->tag()) {
        case Type::Tag::BUFFER:
        case Type::Tag::TEXTURE:
            structSize = type->element()->size();
            break;
        case Type::Tag::BINDLESS_ARRAY:
        case Type::Tag::ACCEL:
            structSize = 0;
            break;
        default:
            structSize = type->size();
            break;
    }
}

Shader::Shader(
    vstd::vector<Property> &&prop,
    vstd::vector<SavedArgument> &&args,
    ComPtr<ID3D12RootSignature> &&rootSig)
    : rootSig(std::move(rootSig)), properties(std::move(prop)), kernelArguments(std::move(args)) {
}

Shader::Shader(
    vstd::vector<Property> &&prop,
    vstd::vector<SavedArgument> &&args,
    ID3D12Device *device,
    bool isRaster) : properties(std::move(prop)), kernelArguments(std::move(args)) {
    auto serializedRootSig = ShaderSerializer::SerializeRootSig(
        properties,
        isRaster);
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
void Shader::SetRasterResource(
    uint propertyName,
    CommandBufferBuilder *cb,
    BufferView buffer) const {
    auto cmdList = cb->CmdList();
    auto &&var = properties[propertyName];
    switch (var.type) {
        case ShaderVariableType::ConstantBuffer: {
            cmdList->SetGraphicsRootConstantBufferView(
                propertyName,
                buffer.buffer->GetAddress() + buffer.offset);
        } break;
        case ShaderVariableType::StructuredBuffer: {
            cmdList->SetGraphicsRootShaderResourceView(
                propertyName,
                buffer.buffer->GetAddress() + buffer.offset);
        } break;
        case ShaderVariableType::RWStructuredBuffer: {
            cmdList->SetGraphicsRootUnorderedAccessView(
                propertyName,
                buffer.buffer->GetAddress() + buffer.offset);
        } break;
    }
}
void Shader::SetRasterResource(
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
            cmdList->SetGraphicsRootDescriptorTable(
                propertyName,
                view.heap->hGPU(view.index));
        } break;
    }
}
void Shader::SetRasterResource(
    uint propertyName,
    CommandBufferBuilder *cmdList,
    TopAccel const *bAccel) const {
    return SetRasterResource(
        propertyName,
        cmdList,
        BufferView(bAccel->GetAccelBuffer()));
}

}// namespace toolhub::directx