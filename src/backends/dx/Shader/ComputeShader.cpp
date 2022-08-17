
#include <Shader/ComputeShader.h>
#include <Shader/ShaderSerializer.h>
#include <vstl/BinaryReader.h>
#include <compile/hlsl/dx_codegen.h>
#include <Shader/ShaderCompiler.h>
#include <vstl/MD5.h>
namespace toolhub::directx {
namespace ComputeShaderDetail {
static void SavePSO(vstd::string_view fileName, FileIO* fileStream, ComputeShader const* cs) {
	ComPtr<ID3DBlob> psoCache;
	cs->Pso()->GetCachedBlob(&psoCache);
	fileStream->write_cache(fileName, {reinterpret_cast<std::byte const*>(psoCache->GetBufferPointer()), psoCache->GetBufferSize()});
};
}// namespace ComputeShaderDetail
ComputeShader* ComputeShader::LoadPresetCompute(
	FileIO* fileIo,
	Device* device,
	vstd::span<Type const* const> types,
	vstd::string_view cacheFolder,
	vstd::string_view psoFolder,
	vstd::string_view fileName) {
	using namespace ComputeShaderDetail;
	bool oldDeleted = false;
	auto result = ShaderSerializer::DeSerialize(
		fileName,
		device,
		*fileIo,
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
				auto& srcType = args[i];
				auto dstType = SavedArgument{types[i]};
				if (srcType.structSize != dstType.structSize ||
					srcType.tag != dstType.tag) {
					return DeleteFunc();
				}
			}
		}
		if (oldDeleted) {
			SavePSO(fileName, fileIo, result);
		}
	}
	return result;
}
ComputeShader* ComputeShader::CompileCompute(
	FileIO* fileIo,
	Device* device,
	Function kernel,
	vstd::function<CodegenResult()> const& codegen,
	uint3 blockSize,
	uint shaderModel,
	vstd::string_view cacheFolder,
	vstd::string_view psoFolder,
	vstd::string_view fileName,
	bool tryLoadOld) {
	using namespace ComputeShaderDetail;
	bool saveCacheFile;
	static constexpr bool PRINT_CODE = false;

	auto CompileNewCompute = [&]<bool WriteCache>() {
		auto str = codegen();
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
			vstd::UndefEval<ComputeShader*>{},
			[&](vstd::unique_ptr<DXByteBlob> const& buffer) {
				auto kernelArgs = ShaderSerializer::SerializeKernel(kernel);
				if constexpr (WriteCache) {
					auto serData = ShaderSerializer::Serialize(
						str.properties,
						kernelArgs,
						{buffer->GetBufferPtr(), buffer->GetBufferSize()},
						str.bdlsBufferCount,
						blockSize);
					fileIo->write_bytecode(fileName, {reinterpret_cast<std::byte const*>(serData.data()), serData.byte_size()});
				}
				auto cs = new ComputeShader(
					blockSize,
					std::move(str.properties),
					std::move(kernelArgs),
					{buffer->GetBufferPtr(),
					 buffer->GetBufferSize()},
					device);
				cs->bindlessCount = str.bdlsBufferCount;
				if constexpr (WriteCache) {
					SavePSO(fileName, fileIo, cs);
				}
				return cs;
			},
			[](auto&& err) {
				std::cout << err << '\n';
				VSTL_ABORT();
				return nullptr;
			});
	};
	if (!fileName.empty()) {

		if (tryLoadOld) {
			bool oldDeleted = false;
			//Cached
			auto result = ShaderSerializer::DeSerialize(
				fileName,
				device,
				*fileIo,
				oldDeleted);
			if (result) {
				if (oldDeleted) {
					SavePSO(fileName, fileIo, result);
				}
				return result;
			}
		}
		return CompileNewCompute.operator()<true>();
	} else {

		return CompileNewCompute.operator()<false>();
	}
}

ComputeShader::ComputeShader(
	uint3 blockSize,
	vstd::vector<Property>&& prop,
	vstd::vector<SavedArgument>&& args,
	vstd::span<vbyte const> binData,
	Device* device)
	: Shader(std::move(prop), std::move(args), device->device.Get()),
	  blockSize(blockSize),
	  device(device) {
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = rootSig.Get();
	psoDesc.CS.pShaderBytecode = binData.data();
	psoDesc.CS.BytecodeLength = binData.size();
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(device->device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(pso.GetAddressOf())));
}
ComputeShader::ComputeShader(
	uint3 blockSize,
	Device* device,
	vstd::vector<Property>&& prop,
	vstd::vector<SavedArgument>&& args,
	ComPtr<ID3D12RootSignature>&& rootSig,
	ComPtr<ID3D12PipelineState>&& pso)
	: device(device),
	  blockSize(blockSize),
	  Shader(std::move(prop), std::move(args), std::move(rootSig)),
	  pso(std::move(pso)) {
}

ComputeShader::~ComputeShader() {
}
}// namespace toolhub::directx