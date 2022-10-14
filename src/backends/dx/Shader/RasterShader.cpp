#include <Shader/RasterShader.h>
#include <Resource/DepthBuffer.h>
#include <Shader/ShaderSerializer.h>
#include <vstl/BinaryReader.h>
#include <HLSL/dx_codegen.h>
#include <Shader/ShaderCompiler.h>
#include <vstl/MD5.h>
namespace toolhub::directx {
namespace RasterShaderDetail {
static void SavePSO(vstd::string_view fileName, BinaryIOVisitor *fileStream, RasterShader const *s) {
    ComPtr<ID3DBlob> psoCache;
    s->Pso()->GetCachedBlob(&psoCache);
    fileStream->write_cache(fileName, {reinterpret_cast<std::byte const *>(psoCache->GetBufferPointer()), psoCache->GetBufferSize()});
};
}// namespace RasterShaderDetail
RasterShader::RasterShader(
    Device *device,
    vstd::vector<Property> &&prop,
    vstd::vector<SavedArgument> &&args,
    ComPtr<ID3D12RootSignature> &&rootSig,
    ComPtr<ID3D12PipelineState> &&pso)
    : Shader(std::move(prop), std::move(args), std::move(rootSig)), pso(std::move(pso)) {}
void RasterShader::GetMeshFormatState(
    vstd::vector<D3D12_INPUT_ELEMENT_DESC> &inputLayout,
    MeshFormat const &meshFormat) {
    inputLayout.clear();
    inputLayout.reserve(meshFormat.vertex_attribute_count());
    static auto SemanticName = {
        "POSITION",
        "NORMAL",
        "TANGENT",
        "COLOR",
        "UV",
        "UV",
        "UV",
        "UV"};
    static auto SemanticIndex = {0u, 0u, 0u, 0u, 0u, 1u, 2u, 3u};
    vstd::vector<uint, VEngine_AllocType::VEngine, 4> offsets(meshFormat.vertex_stream_count());
    memset(offsets.data(), 0, offsets.byte_size());
    for (auto i : vstd::range(meshFormat.vertex_stream_count())) {
        auto vec = meshFormat.attributes(i);
        for (auto &&attr : vec) {
            size_t size;
            DXGI_FORMAT format;
            switch (attr.format) {
                case VertexElementFormat::XYZW8UNorm:
                    size = 4;
                    format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    break;
                case VertexElementFormat::XY16UNorm:
                    size = 4;
                    format = DXGI_FORMAT_R16G16_UNORM;
                    break;
                case VertexElementFormat::XYZW16UNorm:
                    size = 8;
                    format = DXGI_FORMAT_R16G16B16A16_UNORM;
                    break;

                case VertexElementFormat::XY16Float:
                    size = 4;
                    format = DXGI_FORMAT_R16G16_FLOAT;
                    break;
                case VertexElementFormat::XYZW16Float:
                    size = 8;
                    format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                    break;
                case VertexElementFormat::X32Float:
                    size = 4;
                    format = DXGI_FORMAT_R32_FLOAT;
                    break;
                case VertexElementFormat::XY32Float:
                    size = 8;
                    format = DXGI_FORMAT_R32G32_FLOAT;
                    break;
                case VertexElementFormat::XYZ32Float:
                    size = 12;
                    format = DXGI_FORMAT_R32G32B32_FLOAT;
                    break;
                case VertexElementFormat::XYZW32Float:
                    size = 16;
                    format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                    break;
            }
            auto &offset = offsets[i];
            inputLayout.emplace_back(D3D12_INPUT_ELEMENT_DESC{
                .SemanticName = SemanticName.begin()[static_cast<uint>(attr.type)],
                .SemanticIndex = SemanticIndex.begin()[static_cast<uint>(attr.type)],
                .AlignedByteOffset = offset,
                .Format = format,
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                .InputSlot = static_cast<uint>(i)});
            offset += size;
        }
        // TODO
    }
}
RasterShader::RasterShader(
    Device *device,
    vstd::vector<Property> &&prop,
    vstd::vector<SavedArgument> &&args,
    MeshFormat const &meshFormat,
    RasterState const &state,
    vstd::span<PixelFormat const> rtv,
    DepthFormat dsv,
    vstd::span<std::byte const> vertBinData,
    vstd::span<std::byte const> pixelBinData)
    : Shader(std::move(prop), std::move(args), device->device.Get(), true) {
    vstd::vector<D3D12_INPUT_ELEMENT_DESC> layouts;
    auto psoState = GetState(
        layouts,
        meshFormat,
        state,
        rtv,
        dsv);
    psoState.pRootSignature = this->rootSig.Get();
    psoState.VS = {.pShaderBytecode = vertBinData.data(), .BytecodeLength = vertBinData.size()};
    psoState.PS = {.pShaderBytecode = pixelBinData.data(), .BytecodeLength = pixelBinData.size()};
    ThrowIfFailed(device->device->CreateGraphicsPipelineState(&psoState, IID_PPV_ARGS(pso.GetAddressOf())));
}
RasterShader::~RasterShader() {}
D3D12_GRAPHICS_PIPELINE_STATE_DESC RasterShader::GetState(
    vstd::vector<D3D12_INPUT_ELEMENT_DESC> &inputLayout,
    MeshFormat const &meshFormat,
    RasterState const &state,
    vstd::span<PixelFormat const> rtv,
    DepthFormat dsv) {
    auto BlendState = [](BlendOp op) {
        switch (op) {
            case BlendOp::One:
                return D3D12_BLEND_ONE;
            case BlendOp::SrcColor:
                return D3D12_BLEND_SRC_COLOR;
            case BlendOp::DstColor:
                return D3D12_BLEND_DEST_COLOR;
            case BlendOp::SrcAlpha:
                return D3D12_BLEND_SRC_ALPHA;
            case BlendOp::DstAlpha:
                return D3D12_BLEND_DEST_ALPHA;
            case BlendOp::OneMinusSrcColor:
                return D3D12_BLEND_INV_SRC_COLOR;
            case BlendOp::OneMinusDstColor:
                return D3D12_BLEND_INV_DEST_COLOR;
            case BlendOp::OneMinusSrcAlpha:
                return D3D12_BLEND_INV_SRC_ALPHA;
            case BlendOp::OneMinusDstAlpha:
                return D3D12_BLEND_INV_DEST_ALPHA;
            default:
                return D3D12_BLEND_ZERO;
        }
    };
    auto ComparisonState = [](Comparison c) {
        switch (c) {
            case Comparison::Never:
                return D3D12_COMPARISON_FUNC_NEVER;
            case Comparison::Less:
                return D3D12_COMPARISON_FUNC_LESS;
            case Comparison::Equal:
                return D3D12_COMPARISON_FUNC_EQUAL;
            case Comparison::LessEqual:
                return D3D12_COMPARISON_FUNC_LESS_EQUAL;
            case Comparison::Greater:
                return D3D12_COMPARISON_FUNC_GREATER;
            case Comparison::NotEqual:
                return D3D12_COMPARISON_FUNC_NOT_EQUAL;
            case Comparison::GreaterEqual:
                return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
            default:
                return D3D12_COMPARISON_FUNC_ALWAYS;
        }
    };
    auto StencilOpState = [](StencilOp s) {
        switch (s) {
            case StencilOp::Keep:
                return D3D12_STENCIL_OP_KEEP;
            case StencilOp::Replace:
                return D3D12_STENCIL_OP_REPLACE;
            default:
                return D3D12_STENCIL_OP_ZERO;
        }
    };
    auto GetCullMode = [](CullMode c) {
        switch (c) {
            case CullMode::None:
                return D3D12_CULL_MODE_NONE;
            case CullMode::Front:
                return D3D12_CULL_MODE_FRONT;
            default:
                return D3D12_CULL_MODE_BACK;
        }
    };
    auto GetTopology = [](TopologyType t) {
        switch (t) {
            case TopologyType::Point:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
            case TopologyType::Line:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
            default:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        }
    };

    D3D12_RENDER_TARGET_BLEND_DESC blend{};
    if (state.blend_state.enableBlend) {
        auto &v = state.blend_state;
        blend.BlendEnable = true;
        blend.BlendOp = D3D12_BLEND_OP_ADD;
        blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blend.SrcBlend = BlendState(v.src_op);
        blend.DestBlend = BlendState(v.dst_op);
        blend.SrcBlendAlpha = blend.SrcBlend;
        blend.DestBlendAlpha = blend.DestBlend;
        blend.RenderTargetWriteMask = std::numeric_limits<vbyte>::max();
    }
    D3D12_GRAPHICS_PIPELINE_STATE_DESC result = {
        .BlendState = {
            .AlphaToCoverageEnable = false,
            .IndependentBlendEnable = false,
            .RenderTarget = {blend}},
        .PrimitiveTopologyType = GetTopology(state.topology),
        .SampleDesc = {.Count = 1, .Quality = 0},
        .NumRenderTargets = static_cast<uint>(rtv.size())};
    D3D12_DEPTH_STENCIL_DESC &depth = result.DepthStencilState;
    if (state.depth_state.enableDepth) {
        auto &v = state.depth_state;
        depth.DepthEnable = true;
        depth.DepthFunc = ComparisonState(v.comparison);
        depth.DepthWriteMask = v.write ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    }
    if (state.stencil_state.enableStencil) {
        auto &v = state.stencil_state;
        depth.StencilEnable = true;
        depth.StencilReadMask = v.read_mask;
        depth.StencilWriteMask = v.write_mask;
        auto SetFace = [&](StencilFaceOp const &face, D3D12_DEPTH_STENCILOP_DESC &r) {
            r.StencilFunc = ComparisonState(face.comparison);
            r.StencilFailOp = StencilOpState(face.stencil_fail_op);
            r.StencilDepthFailOp = StencilOpState(face.depth_fail_op);
            r.StencilPassOp = StencilOpState(face.pass_op);
        };
        SetFace(v.front_face_op, depth.FrontFace);
        SetFace(v.back_face_op, depth.BackFace);
    }
    result.RasterizerState = {
        .FillMode = state.fill_mode == FillMode::Solid ? D3D12_FILL_MODE_SOLID : D3D12_FILL_MODE_WIREFRAME,
        .CullMode = GetCullMode(state.cull_mode),
        .FrontCounterClockwise = state.front_counter_clockwise,
        .DepthClipEnable = state.depth_clip,
        .ConservativeRaster = state.conservative ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF};
    for (auto i : vstd::range(rtv.size())) {
        result.RTVFormats[i] = static_cast<DXGI_FORMAT>(TextureBase::ToGFXFormat(rtv[i]));
    }
    result.DSVFormat = static_cast<DXGI_FORMAT>(DepthBuffer::GetDepthFormat(dsv));
    GetMeshFormatState(inputLayout, meshFormat);
    result.InputLayout = {.pInputElementDescs = inputLayout.data(), .NumElements = static_cast<uint>(inputLayout.size())};
    return result;
}
vstd::MD5 RasterShader::GenMD5(
    vstd::MD5 const &codeMD5,
    MeshFormat const &meshFormat,
    RasterState const &state,
    vstd::span<PixelFormat const> rtv,
    DepthFormat dsv) {
    vstd::vector<uint64, VEngine_AllocType::VEngine, 8> streamHashes;
    streamHashes.push_back_func(
        meshFormat.vertex_stream_count(),
        [&](size_t i) {
            auto atr = meshFormat.attributes(i);
            return vstd_xxhash_gethash(atr.data(), atr.size_bytes());
        });
    struct Hashes {
        vstd::MD5 codeMd5;
        uint64 meshFormatMD5;
        uint64 rasterStateMD5;
        uint64 rtv;
        uint64 dsv;
    };
    Hashes h{
        .codeMd5 = codeMD5,
        .rasterStateMD5 = vstd_xxhash_gethash(&state, sizeof(RasterState)),
        .rtv = vstd_xxhash_gethash(rtv.data(), rtv.size_bytes()),
        .dsv = vstd_xxhash_gethash(&dsv, sizeof(DepthFormat)),
        .meshFormatMD5 = vstd_xxhash_gethash(streamHashes.data(), streamHashes.byte_size())};
    return vstd::MD5(
        vstd::span<vbyte const>{
            reinterpret_cast<vbyte const *>(&h),
            sizeof(Hashes)});
}
RasterShader *RasterShader::CompileRaster(
    BinaryIOVisitor *fileIo,
    Device *device,
    Function vertexKernel,
    Function pixelKernel,
    vstd::function<CodegenResult()> const &codegen,
    vstd::MD5 const &md5,
    uint shaderModel,
    MeshFormat const &meshFormat,
    RasterState const &state,
    vstd::span<PixelFormat const> rtv,
    DepthFormat dsv,
    vstd::string_view fileName,
    bool byteCodeIsCache) {
    static constexpr bool PRINT_CODE = false;

    auto CompileNewCompute = [&](bool writeCache) -> RasterShader * {
        auto str = codegen();
        if constexpr (PRINT_CODE) {
            std::cout
                << "\n===============================\n"
                << str.result
                << "\n===============================\n";
        }
        auto compResult = Device::Compiler()->CompileRaster(
            str.result,
            true,
            shaderModel);
        if (compResult.vertex.IsTypeOf<vstd::string>()) {
            std::cout << compResult.vertex.get<1>() << '\n';
            VSTL_ABORT();
            return nullptr;
        }
        if (compResult.pixel.IsTypeOf<vstd::string>()) {
            std::cout << compResult.pixel.get<1>() << '\n';
            VSTL_ABORT();
            return nullptr;
        }
        auto kernelArgs = [&] -> vstd::vector<SavedArgument> {
            if (vertexKernel.builder() == nullptr || pixelKernel.builder() == nullptr) {
                return {};
            } else {
                auto vertArgs =
                    vstd::CacheEndRange(vertexKernel.arguments()) |
                    vstd::TransformRange(
                        [&](Variable const &var) {
                            return std::pair<Variable, Usage>{var, vertexKernel.variable_usage(var.uid())};
                        });
                auto pixelSpan = pixelKernel.arguments();
                auto pixelArgs =
                    vstd::ite_range(pixelSpan.begin() + 1, pixelSpan.end()) |
                    vstd::TransformRange(
                        [&](Variable const &var) {
                            return std::pair<Variable, Usage>{var, pixelKernel.variable_usage(var.uid())};
                        });
                auto args = vstd::RangeImpl(vstd::PairIterator(vertArgs, pixelArgs));
                return ShaderSerializer::SerializeKernel(args);
            }
        }();
        auto GetSpan = [&](DXByteBlob const &blob) {
            return vstd::span<std::byte const>{blob.GetBufferPtr(), blob.GetBufferSize()};
        };
        auto vertBin = GetSpan(*compResult.vertex.get<0>());
        auto pixelBin = GetSpan(*compResult.pixel.get<0>());
        if (writeCache) {
            /*
            vstd::vector<D3D12_INPUT_ELEMENT_DESC> elements;
            vstd::vector<InputElement> inputElement;
            auto psoState = GetState(elements, meshFormat, state, rtv, dsv);
            inputElement.push_back_func(
                elements.size(),
                [&](size_t i){
                    auto& src = elements[i];
                    return InputElement{
                        .inputSlot = src.InputSlot,
                        .alignedByteOffset = src.AlignedByteOffset,
                        .format = src.Format,
                        .semanticIndex = src.SemanticIndex,
                        .type
                    }
                }
            )
            RasterHeaderData headerData = {
                psoState.BlendState,
                psoState.RasterizerState,
                psoState.DepthStencilState,
                psoState.PrimitiveTopologyType,
                psoState.NumRenderTargets,
                static_cast<uint>(elements.size())};
            memcpy(&headerData.RTVFormats, psoState.RTVFormats, 8 * sizeof(DXGI_FORMAT));
            headerData.DSVFormat = psoState.DSVFormat;
            auto serData = ShaderSerializer::RasterSerialize(
                str.properties,
                kernelArgs,
                vertBin,
                pixelBin,
                md5,
                str.bdlsBufferCount,
                headerData,
                elements.data());
            if (byteCodeIsCache) {
                fileIo->write_cache(fileName, {reinterpret_cast<std::byte const *>(serData.data()), serData.byte_size()});
            } else {
                fileIo->write_bytecode(fileName, {reinterpret_cast<std::byte const *>(serData.data()), serData.byte_size()});
            }*/
        }

        auto s = new RasterShader(
            device,
            std::move(str.properties),
            std::move(kernelArgs),
            meshFormat,
            state,
            rtv,
            dsv,
            vertBin,
            pixelBin);
        s->bindlessCount = str.bdlsBufferCount;

        if (writeCache) {
            RasterShaderDetail::SavePSO(md5.ToString(), fileIo, s);
        }
        return s;
    };
    if (!fileName.empty()) {
        bool oldDeleted = false;
        auto result = ShaderSerializer::RasterDeSerialize(fileName, byteCodeIsCache, device, *fileIo, {md5}, oldDeleted);
        if (result) {
            if (oldDeleted) {
                RasterShaderDetail::SavePSO(md5.ToString(), fileIo, result);
            }
        }
        return CompileNewCompute(true);
    } else {
        return CompileNewCompute(false);
    }
}
}// namespace toolhub::directx