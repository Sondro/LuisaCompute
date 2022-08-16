
#include <Shader/ComputeShader.h>
#include <Shader/ShaderSerializer.h>
#include <vstl/BinaryReader.h>
#include <compile/hlsl/dx_codegen.h>
#include <Shader/ShaderCompiler.h>
#include <vstl/MD5.h>
namespace toolhub::directx {
namespace ComputeShaderDetail {
struct SerializeVisitor : ShaderSerializer::Visitor {
	BinaryReader csoReader;
	vstd::string const& psoPath;
	bool oldDeleted = false;
	SerializeVisitor(
		vstd::string const& path,
		vstd::string const& psoPath)
		: csoReader(path),
		  psoPath(psoPath) {
	}
	vstd::vector<vbyte> readCache;
	vbyte const* ReadFile(size_t size) override {
		readCache.clear();
		readCache.resize(size);
		csoReader.Read(reinterpret_cast<char*>(readCache.data()), size);
		return readCache.data();
	}
	void ReadFile(void* ptr, size_t size) override {
		csoReader.Read(ptr, size);
	}
	ShaderSerializer::ReadResult ReadFileAndPSO(
		size_t fileSize) override {
		BinaryReader psoReader(psoPath);
		ShaderSerializer::ReadResult result;
		readCache.clear();
		if (psoReader) {
			size_t psoSize = psoReader.GetLength();
			readCache.resize(psoSize + fileSize);
			result.fileSize = fileSize;
			result.fileData = readCache.data();
			result.psoSize = psoSize;
			result.psoData = readCache.data() + fileSize;
			csoReader.Read(reinterpret_cast<char*>(readCache.data()), fileSize);
			psoReader.Read(reinterpret_cast<char*>(readCache.data() + fileSize), psoSize);
		} else {
			oldDeleted = true;
			readCache.resize(fileSize);
			result.fileSize = fileSize;
			result.fileData = readCache.data();
			result.psoSize = 0;
			result.psoData = nullptr;
			csoReader.Read(reinterpret_cast<char*>(readCache.data()), fileSize);
		}
		return result;
	}
	void DeletePSOFile() override {
		oldDeleted = true;
	}
};
static void SavePSO(vstd::string const& psoPath, ComputeShader const* cs) {
	auto f = fopen(psoPath.c_str(), "wb");
	if (f) {
		auto disp = vstd::create_disposer([&] { fclose(f); });
		ComPtr<ID3DBlob> psoCache;
		cs->Pso()->GetCachedBlob(&psoCache);
		fwrite(psoCache->GetBufferPointer(), psoCache->GetBufferSize(), 1, f);
	}
};
}// namespace ComputeShaderDetail
ComputeShader* ComputeShader::LoadPresetCompute(
	Device* device,
	vstd::span<Type const* const> types,
	vstd::string_view cacheFolder,
	vstd::string_view psoFolder,
	vstd::string_view fileName) {
	using namespace ComputeShaderDetail;
	vstd::string path;
	path << cacheFolder << fileName;
	vstd::string psoPath;
	psoPath << psoFolder << fileName;
	SerializeVisitor visitor(
		path,
		psoPath);
	//Cached
	if (visitor.csoReader) {
		auto result = ShaderSerializer::DeSerialize(
			device,
			visitor);
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
			if (visitor.oldDeleted) {
				SavePSO(psoPath, result);
			}
			return result;
		}
	}
	return nullptr;
}
ComputeShader* ComputeShader::CompileCompute(
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

	auto CompileNewCompute = [&]<bool WriteCache>(vstd::string const* path, vstd::string const* psoPath) {
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
					auto f = fopen(path->c_str(), "wb");
					if (f) {
						auto disp = vstd::create_disposer([&] { fclose(f); });
						auto serData = ShaderSerializer::Serialize(
							str.properties,
							kernelArgs,
							{buffer->GetBufferPtr(), buffer->GetBufferSize()},
							str.bdlsBufferCount,
							blockSize);
						fwrite(serData.data(), serData.size(), 1, f);
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
				if constexpr (WriteCache) {
					SavePSO(*psoPath, cs);
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
		auto ProcessPath = [&](vstd::string_view folder) {
			vstd::string p(folder);
			p << fileName;
			return p;
		};
		vstd::string path = ProcessPath(cacheFolder);
		vstd::string psoPath = ProcessPath(psoFolder);
		if (tryLoadOld) {
			SerializeVisitor visitor(
				path,
				psoPath);
			//Cached
			if (visitor.csoReader) {
				auto result = ShaderSerializer::DeSerialize(
					device,
					visitor);
				if (result) {
					if (visitor.oldDeleted) {
						SavePSO(psoPath, result);
					}
					return result;
				}
			}
		}
		return CompileNewCompute.operator()<true>(&path, &psoPath);
	} else {

		return CompileNewCompute.operator()<false>(nullptr, nullptr);
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