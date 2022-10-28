#include <Shader/ComputeShader.h>
#include <Shader/ShaderSerializer.h>
#include <vstl/BinaryReader.h>
#include <HLSL/dx_codegen.h>
#include <Shader/ShaderCompiler.h>
#include <vstl/MD5.h>
namespace toolhub::directx {
namespace ComputeShaderDetail {

static void SavePSO(vstd::string_view fileName, BinaryIOVisitor *fileStream, ComputeShader const *cs) {
    vstd::MD5 fileNameMd5(fileName);
    ComPtr<ID3DBlob> psoCache;
    cs->Pso()->GetCachedBlob(&psoCache);
    auto name = fileNameMd5.ToString(false);
    fileStream->write_cache(name, {reinterpret_cast<std::byte const *>(psoCache->GetBufferPointer()), psoCache->GetBufferSize()});
};
}// namespace ComputeShaderDetail
ComputeShader *ComputeShader::LoadPresetCompute(
    BinaryIOVisitor *fileIo,
    Device *device,
    vstd::span<Type const *const> types,
    vstd::string_view fileName) {
    using namespace ComputeShaderDetail;
    bool oldDeleted = false;
    auto result = ShaderSerializer::DeSerialize(
        fileName,
        false,
        device,
        *fileIo,
        {},
        oldDeleted);
    //Cached

    if (result) {
        // args check
        auto args = result->Args();
        auto DeleteFunc = [&] {
            delete result;
            return nullptr;
        };
        if (args.size() != types.size()) {
            return DeleteFunc();
        } else {
            for (auto i : vstd::range(args.size())) {
                auto &srcType = args[i];
                auto dstType = SavedArgument{types[i]};
                if (srcType.structSize != dstType.structSize ||
                    srcType.tag != dstType.tag) {
                    return DeleteFunc();
                }
            }
        }
        if (oldDeleted) {
            // SavePSO(lastMD5.ToString(), fileIo, result);
            SavePSO(fileName, fileIo, result);
        }
    }
    return result;
}
ComputeShader *ComputeShader::CompileCompute(
    BinaryIOVisitor *fileIo,
    Device *device,
    Function kernel,
    vstd::function<CodegenResult()> const &codegen,
    vstd::optional<vstd::MD5> const &checkMD5,
    uint3 blockSize,
    uint shaderModel,
    vstd::string_view fileName,
    bool byteCodeIsCache) {
    using namespace ComputeShaderDetail;
    static constexpr bool PRINT_CODE = false;

    auto CompileNewCompute = [&](bool WriteCache) {
        auto str = codegen();
        vstd::MD5 md5;
        if (WriteCache) {
            if (checkMD5) {
                md5 = *checkMD5;
            } else {
                md5 = vstd::MD5({reinterpret_cast<uint8_t const *>(str.result.data() + str.immutableHeaderSize), str.result.size() - str.immutableHeaderSize});
            }
        }
        if constexpr (PRINT_CODE) {
            std::cout
                << "\n===============================\n"
                << str.result
                << "\n===============================\n";
        }
        auto compResult = Device::Compiler()->CompileCompute(
            str.result,
            true,
            shaderModel);

        return compResult.multi_visit_or(
            vstd::UndefEval<ComputeShader *>{},
            [&](vstd::unique_ptr<DXByteBlob> const &buffer) {
                auto kernelArgs = [&] {
                    if (kernel.builder() == nullptr) {
                        return vstd::vector<SavedArgument>();
                    } else {
                        return ShaderSerializer::SerializeKernel(kernel);
                    }
                }();
                if (WriteCache) {
                    auto serData = ShaderSerializer::Serialize(
                        str.properties,
                        kernelArgs,
                        {buffer->GetBufferPtr(), buffer->GetBufferSize()},
                        md5,
                        str.bdlsBufferCount,
                        blockSize);
                    if (byteCodeIsCache) {
                        fileIo->write_cache(fileName, {reinterpret_cast<std::byte const *>(serData.data()), serData.size_bytes()});
                    } else {
                        fileIo->write_bytecode(fileName, {reinterpret_cast<std::byte const *>(serData.data()), serData.size_bytes()});
                    }
                }
                auto cs = new ComputeShader(
                    blockSize,
                    std::move(str.properties),
                    std::move(kernelArgs),
                    {buffer->GetBufferPtr(),
                     buffer->GetBufferSize()},
                    device);
                cs->bindlessCount = str.bdlsBufferCount;
                if (WriteCache) {
                    SavePSO(fileName, fileIo, cs);
                    // SavePSO(md5.ToString(), fileIo, cs);
                }
                return cs;
            },
            [](auto &&err) {
                std::cout << err << '\n';
                VSTL_ABORT();
                return nullptr;
            });
    };
    if (!fileName.empty()) {

        bool oldDeleted = false;
        //Cached
        auto result = ShaderSerializer::DeSerialize(
            fileName,
            byteCodeIsCache,
            device,
            *fileIo,
            checkMD5,
            oldDeleted);
        if (result) {
            if (oldDeleted) {
                SavePSO(fileName, fileIo, result);
                // SavePSO(lastMD5.ToString(), fileIo, result);
            }
            return result;
        }

        return CompileNewCompute(true);
    } else {
        return CompileNewCompute(false);
    }
}
void ComputeShader::SaveCompute(
    BinaryIOVisitor *fileIo,
    Function kernel,
    CodegenResult &str,
    uint3 blockSize,
    uint shaderModel,
    vstd::string_view fileName) {
    using namespace ComputeShaderDetail;
    vstd::MD5 md5({reinterpret_cast<uint8_t const *>(str.result.data() + str.immutableHeaderSize), str.result.size() - str.immutableHeaderSize});
    if (ShaderSerializer::CheckMD5(fileName, md5, *fileIo)) return;
    auto compResult = Device::Compiler()->CompileCompute(
        str.result,
        true,
        shaderModel);
    compResult.multi_visit(
        [&](vstd::unique_ptr<DXByteBlob> const &buffer) {
            auto kernelArgs = ShaderSerializer::SerializeKernel(kernel);
            auto serData = ShaderSerializer::Serialize(
                str.properties,
                kernelArgs,
                {buffer->GetBufferPtr(), buffer->GetBufferSize()},
                md5,
                str.bdlsBufferCount,
                blockSize);
            fileIo->write_bytecode(fileName, {reinterpret_cast<std::byte const *>(serData.data()), serData.size_bytes()});
        },
        [](auto &&err) {
            std::cout << err << '\n';
            VSTL_ABORT();
        });
}
ID3D12CommandSignature *ComputeShader::CmdSig() const {
    std::lock_guard lck(cmdSigMtx);
    if (cmdSig) return cmdSig.Get();
    D3D12_COMMAND_SIGNATURE_DESC desc{};
    D3D12_INDIRECT_ARGUMENT_DESC indDesc[2];
    memset(indDesc, 0, vstd::array_byte_size(indDesc));
    indDesc[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
    auto &c = indDesc[0].Constant;
    c.RootParameterIndex = 1;
    c.DestOffsetIn32BitValues = 0;
    c.Num32BitValuesToSet = 4;
    indDesc[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
    desc.ByteStride = 28;
    desc.NumArgumentDescs = 2;
    desc.pArgumentDescs = indDesc;
    ThrowIfFailed(device->device->CreateCommandSignature(&desc, rootSig.Get(), IID_PPV_ARGS(&cmdSig)));
    return cmdSig.Get();
}

ComputeShader::ComputeShader(
    uint3 blockSize,
    vstd::vector<Property> &&prop,
    vstd::vector<SavedArgument> &&args,
    vstd::span<std::byte const> binData,
    Device *device)
    : Shader(std::move(prop), std::move(args), device->device.Get(), false),
      device(device),
      blockSize(blockSize) {
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSig.Get();
    psoDesc.CS.pShaderBytecode = binData.data();
    psoDesc.CS.BytecodeLength = binData.size();
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(device->device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(pso.GetAddressOf())));
}
ComputeShader::ComputeShader(
    uint3 blockSize,
    Device *device,
    vstd::vector<Property> &&prop,
    vstd::vector<SavedArgument> &&args,
    ComPtr<ID3D12RootSignature> &&rootSig,
    ComPtr<ID3D12PipelineState> &&pso)
    : Shader(std::move(prop), std::move(args), std::move(rootSig)),
      pso(std::move(pso)),
      device(device),
      blockSize(blockSize) {
}

ComputeShader::~ComputeShader() {
}
}// namespace toolhub::directx