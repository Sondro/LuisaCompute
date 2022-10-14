
#include <Shader/ShaderSerializer.h>
#include <Shader/ComputeShader.h>
#include <DXRuntime/GlobalSamplers.h>
#include <vstl/small_vector.h>
namespace toolhub::directx {
namespace shader_ser {
struct Header {
    vstd::MD5 md5;
    uint64 rootSigBytes;
    uint64 codeBytes;
    uint3 blockSize;
    uint propertyCount;
    uint bindlessCount;
    uint kernelArgCount;
};
}// namespace shader_ser

vstd::vector<std::byte>
ShaderSerializer::Serialize(
    vstd::span<Property const> properties,
    vstd::span<SavedArgument const> kernelArgs,
    vstd::span<std::byte const> binByte,
    vstd::MD5 const &checkMD5,
    uint bindlessCount,
    uint3 blockSize) {
    using namespace shader_ser;
    vstd::vector<std::byte> result;
    result.reserve(65500);
    result.resize(sizeof(Header));
    Header header = {
        checkMD5,
        (uint64)SerializeRootSig(properties, result),
        (uint64)binByte.size(),
        blockSize,
        static_cast<uint>(properties.size()),
        bindlessCount,
        static_cast<uint>(kernelArgs.size())};
    *reinterpret_cast<Header *>(result.data()) = std::move(header);
    result.push_back_all(binByte);
    result.push_back_all(
        reinterpret_cast<std::byte const *>(properties.data()),
        properties.size_bytes());
    result.push_back_all(
        reinterpret_cast<std::byte const *>(kernelArgs.data()),
        kernelArgs.size_bytes());
    return result;
}

bool ShaderSerializer::CheckMD5(
    vstd::string_view fileName,
    vstd::MD5 const &checkMD5,
    BinaryIOVisitor &streamFunc) {
    using namespace shader_ser;
    auto binStream = streamFunc.read_bytecode(fileName);
    if (binStream == nullptr || binStream->length() <= sizeof(Header)) return false;
    Header header;
    binStream->read({reinterpret_cast<std::byte *>(&header),
                     sizeof(Header)});

    return header.md5 == checkMD5;
}
ComputeShader *ShaderSerializer::DeSerialize(
    vstd::string_view name,
    bool byteCodeIsCache,
    Device *device,
    BinaryIOVisitor &streamFunc,
    vstd::optional<vstd::MD5> const &checkMD5,
    bool &clearCache,
    vstd::MD5 *lastMD5) {
    using namespace shader_ser;
    /*
	vstd::small_vector<void*> releaseVec;
	auto disp = vstd::scope_exit([&] {
		for (auto&& i : releaseVec) { vengine_free(i); }
	});
	BinaryIOVisitor::AllocateFunc func = [&](size_t byteSize) {
		auto ptr = vengine_malloc(byteSize);
		releaseVec.emplace_back(ptr);
		return ptr;
	};*/
    auto binStream = [&] {
        if (byteCodeIsCache)
            return streamFunc.read_cache(name);
        else
            return streamFunc.read_bytecode(name);
    }();
    if (binStream == nullptr || binStream->length() <= sizeof(Header)) return nullptr;
    Header header;
    binStream->read({reinterpret_cast<std::byte *>(&header),
                     sizeof(Header)});
    if (lastMD5)
        *lastMD5 = header.md5;

    if (checkMD5 && header.md5 != *checkMD5) return nullptr;
    vstd::vector<std::byte> binCode(binStream->length() - sizeof(Header));
    vstd::vector<std::byte> psoCode;

    binStream->read({binCode.data(), binCode.size()});
    auto psoStream = streamFunc.read_cache(header.md5.ToString());
    if (psoStream != nullptr && psoStream->length() > 0) {
        psoCode.resize(psoStream->length());
        psoStream->read({psoCode.data(), psoCode.size()});
    }
    auto binPtr = binCode.data();
    auto rootSig = DeSerializeRootSig(
        device->device.Get(),
        {reinterpret_cast<std::byte const *>(binPtr), header.rootSigBytes});
    binPtr += header.rootSigBytes;
    // Try pipeline library
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc;
    psoDesc.NodeMask = 0;
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    psoDesc.pRootSignature = rootSig.Get();
    ComPtr<ID3D12PipelineState> pso;
    psoDesc.CS.pShaderBytecode = binPtr;
    psoDesc.CS.BytecodeLength = header.codeBytes;
    binPtr += header.codeBytes;
    psoDesc.CachedPSO.CachedBlobSizeInBytes = psoCode.size();
    auto createPipe = [&] {
        return device->device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(pso.GetAddressOf()));
    };
    // use PSO cache
    if (psoCode.empty()) {
        // No PSO
        clearCache = true;
        psoDesc.CachedPSO.pCachedBlob = nullptr;
        ThrowIfFailed(createPipe());
    } else {
        psoDesc.CachedPSO.pCachedBlob = psoCode.data();
        auto psoGenSuccess = createPipe();
        if (psoGenSuccess != S_OK) {
            // PSO cache miss(probably driver's version or hardware transformed), discard cache
            clearCache = true;
            if (pso == nullptr) {
                psoDesc.CachedPSO.CachedBlobSizeInBytes = 0;
                psoDesc.CachedPSO.pCachedBlob = nullptr;
                ThrowIfFailed(createPipe());
            }
        }
    }
    vstd::vector<Property> properties;
    vstd::vector<SavedArgument> kernelArgs;
    properties.resize(header.propertyCount);
    kernelArgs.resize(header.kernelArgCount);
    memcpy(properties.data(), binPtr, properties.byte_size());
    binPtr += properties.byte_size();
    memcpy(kernelArgs.data(), binPtr, kernelArgs.byte_size());
    binPtr += kernelArgs.byte_size();

    auto cs = new ComputeShader(
        header.blockSize,
        device,
        std::move(properties),
        std::move(kernelArgs),
        std::move(rootSig),
        std::move(pso));
    cs->bindlessCount = header.bindlessCount;
    return cs;
}
ComPtr<ID3DBlob> ShaderSerializer::SerializeRootSig(
    vstd::span<Property const> properties) {
    vstd::vector<CD3DX12_ROOT_PARAMETER, VEngine_AllocType::VEngine, 32> allParameter;
    allParameter.reserve(properties.size());
    vstd::vector<CD3DX12_DESCRIPTOR_RANGE, VEngine_AllocType::VEngine, 32> allRange;
    for (auto &&var : properties) {
        switch (var.type) {
            case ShaderVariableType::UAVDescriptorHeap:
            case ShaderVariableType::CBVDescriptorHeap:
            case ShaderVariableType::SampDescriptorHeap:
            case ShaderVariableType::SRVDescriptorHeap: {
                allRange.emplace_back();
            } break;
        }
    }
    size_t offset = 0;
    for (auto &&var : properties) {

        switch (var.type) {
            case ShaderVariableType::SRVDescriptorHeap: {
                CD3DX12_DESCRIPTOR_RANGE &range = allRange[offset];
                offset++;
                range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, var.arrSize == 0 ? 1 : var.arrSize, var.registerIndex, var.spaceIndex);
                allParameter.emplace_back().InitAsDescriptorTable(1, &range);
            } break;
            case ShaderVariableType::CBVDescriptorHeap: {
                CD3DX12_DESCRIPTOR_RANGE &range = allRange[offset];
                offset++;
                range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, var.arrSize == 0 ? 1 : var.arrSize, var.registerIndex, var.spaceIndex);
                allParameter.emplace_back().InitAsDescriptorTable(1, &range);
            } break;
            case ShaderVariableType::SampDescriptorHeap: {
                CD3DX12_DESCRIPTOR_RANGE &range = allRange[offset];
                offset++;
                range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, var.arrSize == 0 ? 1 : var.arrSize, var.registerIndex, var.spaceIndex);
                allParameter.emplace_back().InitAsDescriptorTable(1, &range);
            } break;
            case ShaderVariableType::UAVDescriptorHeap: {
                CD3DX12_DESCRIPTOR_RANGE &range = allRange[offset];
                offset++;
                range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, var.arrSize == 0 ? 1 : var.arrSize, var.registerIndex, var.spaceIndex);
                allParameter.emplace_back().InitAsDescriptorTable(1, &range);
            } break;
            case ShaderVariableType::ConstantBuffer:
                allParameter.emplace_back().InitAsConstantBufferView(var.registerIndex, var.spaceIndex);
                break;
            case ShaderVariableType::StructuredBuffer:
                allParameter.emplace_back().InitAsShaderResourceView(var.registerIndex, var.spaceIndex);
                break;
            case ShaderVariableType::RWStructuredBuffer:
                allParameter.emplace_back().InitAsUnorderedAccessView(var.registerIndex, var.spaceIndex);
                break;
            default:
                break;
        }
    }
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc(
        allParameter.size(), allParameter.data(),
        0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    ThrowIfFailed(D3D12SerializeVersionedRootSignature(
        &rootSigDesc,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf()));
    return serializedRootSig;
}
size_t ShaderSerializer::SerializeRootSig(
    vstd::span<Property const> properties,
    vstd::vector<std::byte> &result) {
    auto lastSize = result.size();
    auto blob = SerializeRootSig(properties);
    result.push_back_all(
        (std::byte const *)blob->GetBufferPointer(),
        blob->GetBufferSize());
    return result.size() - lastSize;
}
ComPtr<ID3D12RootSignature> ShaderSerializer::DeSerializeRootSig(
    ID3D12Device *device,
    vstd::span<std::byte const> bytes) {
    ComPtr<ID3D12RootSignature> rootSig;
    ThrowIfFailed(device->CreateRootSignature(
        0,
        bytes.data(),
        bytes.size(),
        IID_PPV_ARGS(rootSig.GetAddressOf())));
    return rootSig;
}
vstd::vector<SavedArgument> ShaderSerializer::SerializeKernel(Function kernel) {
    assert(kernel.tag() == Function::Tag::KERNEL);
    auto &&args = kernel.arguments();
    vstd::vector<SavedArgument> result;
    result.push_back_func(args.size(), [&](size_t i) {
        auto &&var = args[i];
        auto type = var.type();
        return SavedArgument(kernel, var);
    });
    return result;
}
}// namespace toolhub::directx