
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
    void ReadFile(void* ptr, size_t size) override{
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
	uint3 blockSize,
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
			{},
			visitor);
		if (result) {
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
	CodegenResult const& str,
	uint3 blockSize,
	uint shaderModel,
	vstd::string_view cacheFolder,
	vstd::string_view psoFolder,
	vstd::optional<vstd::string>&& fileName) {
	using namespace ComputeShaderDetail;

	auto ProcessPath = [](vstd::string_view folder, vstd::string_view fileName, vstd::optional<vstd::string>& prePath) {
		vstd::string p(folder);
		if (prePath) {
			p << *prePath;
		} else {
			p << fileName;
		}
		return p;
	};
	auto md5Str = str.md5.ToString();
	vstd::string path = ProcessPath(cacheFolder, md5Str, fileName);
	vstd::string psoPath = ProcessPath(psoFolder, md5Str, fileName);
	static constexpr bool USE_CACHE = true;
	if constexpr (USE_CACHE) {
		SerializeVisitor visitor(
			path,
			psoPath);
		//Cached
		if (visitor.csoReader) {
			auto result = ShaderSerializer::DeSerialize(
				device,
				{str.md5},
				visitor);
			if (result) {
				if (visitor.oldDeleted) {
					SavePSO(psoPath, result);
				}
				return result;
			}
		}
	}

	auto compResult = [&] {
		if constexpr (!USE_CACHE) {
			std::cout
				<< "\n===============================\n"
				<< str.result
				<< "\n===============================\n";
		}
		return Device::Compiler()->CompileCompute(
			str.result,
			true,
			shaderModel);
	}();
	return compResult.multi_visit_or(
		(ComputeShader*)nullptr,
		[&](vstd::unique_ptr<DXByteBlob> const& buffer) {
			auto f = fopen(path.c_str(), "wb");
			if (f) {
				if constexpr (USE_CACHE) {
					auto disp = vstd::create_disposer([&] { fclose(f); });
					auto serData = ShaderSerializer::Serialize(
						str.properties,
						{buffer->GetBufferPtr(), buffer->GetBufferSize()},
						str.md5,
                        str.bdlsBufferCount,
						blockSize);
					fwrite(serData.data(), serData.size(), 1, f);
				}
			}
			auto cs = new ComputeShader(
				blockSize,
				str.properties,
				{buffer->GetBufferPtr(),
				 buffer->GetBufferSize()},
				device);
			cs->bindlessCount = str.bdlsBufferCount;
			if constexpr (USE_CACHE) {
				SavePSO(psoPath, cs);
			}
			return cs;
		},
		[](auto&& err) {
			std::cout << err << '\n';
			VSTL_ABORT();
			return nullptr;
		});
}
ComputeShader::ComputeShader(
	uint3 blockSize,
	vstd::span<Property const> properties,
	vstd::span<vbyte const> binData,
	Device* device)
	: Shader(std::move(properties), device->device.Get()),
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
	vstd::span<Property const> prop,
	ComPtr<ID3D12RootSignature>&& rootSig,
	ComPtr<ID3D12PipelineState>&& pso)
	: device(device),
	  blockSize(blockSize),
	  Shader(prop, std::move(rootSig)),
	  pso(std::move(pso)) {
}

ComputeShader::~ComputeShader() {
}
}// namespace toolhub::directx