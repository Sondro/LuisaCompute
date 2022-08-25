#include <Api/LCDevice.h>
#include <DXRuntime/Device.h>
#include <Resource/DefaultBuffer.h>
#include <Shader/ShaderCompiler.h>
#include <Resource/RenderTexture.h>
#include <Resource/BindlessArray.h>
#include <Shader/ComputeShader.h>
#include <Api/LCCmdBuffer.h>
#include <Api/LCEvent.h>
#include <vstl/MD5.h>
#include <Shader/ShaderSerializer.h>
#include <Resource/BottomAccel.h>
#include <Resource/TopAccel.h>
#include <vstl/BinaryReader.h>
#include <Api/LCSwapChain.h>
#include <Api/LCUtil.h>
#include <compile/hlsl/dx_codegen.h>
#include <serialize/serialize.h>
#include <serialize/DatabaseInclude.h>
#include <ast/function_builder.h>
using namespace toolhub::directx;
namespace toolhub::directx {
LCDevice::LCDevice(const Context& ctx)
	: LCDeviceInterface(ctx), nativeDevice(shaderPaths) {
	shaderPaths.shaderCacheFolder = ctx.cache_directory().string().c_str();
	shaderPaths.dataFolder = ctx.data_directory().string().c_str();
	auto ProcessPath = [](vstd::string& str) {
		for (auto&& i : str) {
			if (i == '\\') i = '/';
		}
		if (*(str.end() - 1) != '/') str << '/';
	};
	ProcessPath(shaderPaths.shaderCacheFolder);
	ProcessPath(shaderPaths.dataFolder);
}
LCDevice::~LCDevice() {
}
void* LCDevice::native_handle() const noexcept {
	return nativeDevice.device.Get();
}
uint64 LCDevice::create_buffer(size_t size_bytes) noexcept {
	return reinterpret_cast<uint64>(
		static_cast<Buffer*>(
			new DefaultBuffer(
				&nativeDevice,
				size_bytes,
				nativeDevice.defaultAllocator.get())));
}
void LCDevice::destroy_buffer(uint64 handle) noexcept {
	delete reinterpret_cast<Buffer*>(handle);
}
void* LCDevice::buffer_native_handle(uint64 handle) const noexcept {
	return reinterpret_cast<Buffer*>(handle)->GetResource();
}
uint64 LCDevice::create_texture(
	PixelFormat format,
	uint dimension,
	uint width,
	uint height,
	uint depth,
	uint mipmap_levels) noexcept {
	bool allowUAV = true;
	switch (format) {
		case PixelFormat::BC4UNorm:
		case PixelFormat::BC5UNorm:
		case PixelFormat::BC6HUF16:
		case PixelFormat::BC7UNorm:
			allowUAV = false;
			break;
	}
	return reinterpret_cast<uint64>(
		new RenderTexture(
			&nativeDevice,
			width,
			height,
			TextureBase::ToGFXFormat(format),
			(TextureDimension)dimension,
			depth,
			mipmap_levels,
			allowUAV,
			nativeDevice.defaultAllocator.get()));
}
void LCDevice::destroy_texture(uint64 handle) noexcept {
	delete reinterpret_cast<RenderTexture*>(handle);
}
void* LCDevice::texture_native_handle(uint64 handle) const noexcept {
	return reinterpret_cast<RenderTexture*>(handle)->GetResource();
}
uint64 LCDevice::create_bindless_array(size_t size) noexcept {
	return reinterpret_cast<uint64>(
		new BindlessArray(&nativeDevice, size));
}
void LCDevice::destroy_bindless_array(uint64 handle) noexcept {
	delete reinterpret_cast<BindlessArray*>(handle);
}
void LCDevice::emplace_buffer_in_bindless_array(uint64 array, size_t index, uint64 handle, size_t offset_bytes) noexcept {
	auto buffer = reinterpret_cast<Buffer*>(handle);
	reinterpret_cast<BindlessArray*>(array)
		->Bind(handle, BufferView(buffer, offset_bytes), index);
}
void LCDevice::emplace_tex2d_in_bindless_array(uint64 array, size_t index, uint64 handle, Sampler sampler) noexcept {
	auto tex = reinterpret_cast<RenderTexture*>(handle);
	reinterpret_cast<BindlessArray*>(array)
		->Bind(handle, std::pair<TextureBase const*, Sampler>(tex, sampler), index);
}
void LCDevice::emplace_tex3d_in_bindless_array(uint64 array, size_t index, uint64 handle, Sampler sampler) noexcept {
	emplace_tex2d_in_bindless_array(array, index, handle, sampler);
}
/*
bool LCDevice::is_resource_in_bindless_array(uint64 array, uint64 handle) const noexcept {

}*/
void LCDevice::remove_buffer_in_bindless_array(uint64 array, size_t index) noexcept {
	reinterpret_cast<BindlessArray*>(array)
		->UnBind(BindlessArray::BindTag::Buffer, index);
}
void LCDevice::remove_tex2d_in_bindless_array(uint64 array, size_t index) noexcept {
	reinterpret_cast<BindlessArray*>(array)
		->UnBind(BindlessArray::BindTag::Tex2D, index);
}
void LCDevice::remove_tex3d_in_bindless_array(uint64 array, size_t index) noexcept {
	reinterpret_cast<BindlessArray*>(array)
		->UnBind(BindlessArray::BindTag::Tex3D, index);
}
uint64 LCDevice::create_stream(bool allowPresent) noexcept {
	return reinterpret_cast<uint64>(
		new LCCmdBuffer(
			&nativeDevice,
			nativeDevice.defaultAllocator.get(),
			allowPresent ? D3D12_COMMAND_LIST_TYPE_DIRECT : D3D12_COMMAND_LIST_TYPE_COMPUTE));
}

void LCDevice::destroy_stream(uint64 handle) noexcept {
	delete reinterpret_cast<LCCmdBuffer*>(handle);
}
void LCDevice::synchronize_stream(uint64 stream_handle) noexcept {
	reinterpret_cast<LCCmdBuffer*>(stream_handle)->Sync();
}
void LCDevice::dispatch(uint64 stream_handle, CommandList&& list) noexcept {
	reinterpret_cast<LCCmdBuffer*>(stream_handle)
		->Execute(std::move(list), maxAllocatorCount);
}
void LCDevice::dispatch(uint64 stream_handle, luisa::move_only_function<void()>&& func) noexcept {
	reinterpret_cast<LCCmdBuffer*>(stream_handle)->queue.Callback(std::move(func));
}

void* LCDevice::stream_native_handle(uint64 handle) const noexcept {
	return reinterpret_cast<LCCmdBuffer*>(handle)
		->queue.Queue();
}
void LCDevice::set_io_visitor(BinaryIOVisitor* visitor) noexcept {
	if (visitor) {
		nativeDevice.fileIo = visitor;
	} else {
		nativeDevice.fileIo = &nativeDevice.serVisitor;
	}
}

uint64 LCDevice::create_shader(Function kernel, std::string_view file_name) noexcept {
	//auto code = CodegenUtility::Codegen(kernel, shaderPaths.dataFolder);
	AstSerializer ser{};
	auto db = vstd::create_unique(CreateDatabase());
	ser.SerializeKernel(kernel, db.get());
	luisa::compute::detail::FunctionBuilder builder(Function::Tag::KERNEL);
	ser.DeserializeKernel(&builder, db->GetRootNode());
	auto code = CodegenUtility::Codegen(Function(&builder), shaderPaths.dataFolder);
	vstd::MD5 md5({reinterpret_cast<vbyte const*>(code.result.data() + code.immutableHeaderSize), code.result.size() - code.immutableHeaderSize});
	return reinterpret_cast<uint64>(
		ComputeShader::CompileCompute(
			nativeDevice.fileIo,
			&nativeDevice,
			kernel,
			[&] -> decltype(auto) { return std::move(code); },
			kernel.block_size(),
			65u,
			file_name,
			md5));
}
uint64 LCDevice::load_shader(
	vstd::string_view file_name,
	vstd::span<Type const* const> types) noexcept {
	return reinterpret_cast<uint64>(
		ComputeShader::LoadPresetCompute(
			nativeDevice.fileIo,
			&nativeDevice,
			types,
			file_name));
}

void LCDevice::destroy_shader(uint64 handle) noexcept {
	auto shader = reinterpret_cast<Shader*>(handle);
	delete shader;
}
uint64 LCDevice::create_event() noexcept {
	return reinterpret_cast<uint64>(
		new LCEvent(&nativeDevice));
}
void LCDevice::destroy_event(uint64 handle) noexcept {
	delete reinterpret_cast<LCEvent*>(handle);
}
void LCDevice::signal_event(uint64 handle, uint64 stream_handle) noexcept {
	reinterpret_cast<LCEvent*>(handle)->Signal(
		&reinterpret_cast<LCCmdBuffer*>(stream_handle)->queue);
}

void LCDevice::wait_event(uint64 handle, uint64 stream_handle) noexcept {
	reinterpret_cast<LCEvent*>(handle)->Wait(
		&reinterpret_cast<LCCmdBuffer*>(stream_handle)->queue);
}
void LCDevice::synchronize_event(uint64 handle) noexcept {
	reinterpret_cast<LCEvent*>(handle)->Sync();
}

uint64 LCDevice::create_mesh(
	AccelUsageHint hint,
	bool allow_compact, bool allow_update) noexcept {
	return reinterpret_cast<uint64>(
		(
			new BottomAccel(
				&nativeDevice,
				hint,
				allow_compact,
				allow_update)));
}
void LCDevice::destroy_mesh(uint64 handle) noexcept {
	delete reinterpret_cast<BottomAccel*>(handle);
}
uint64 LCDevice::create_accel(AccelUsageHint hint, bool allow_compact, bool allow_update) noexcept {
	return reinterpret_cast<uint64>(new TopAccel(
		&nativeDevice,
		hint,
		allow_compact,
		allow_update));
}
void LCDevice::destroy_accel(uint64 handle) noexcept {
	delete reinterpret_cast<TopAccel*>(handle);
}
uint64 LCDevice::create_swap_chain(
	uint64 window_handle,
	uint64 stream_handle,
	uint width,
	uint height,
	bool allow_hdr,
	uint back_buffer_size) noexcept {
	return reinterpret_cast<uint64>(
		new LCSwapChain(
			&nativeDevice,
			&reinterpret_cast<LCCmdBuffer*>(stream_handle)->queue,
			nativeDevice.defaultAllocator.get(),
			reinterpret_cast<HWND>(window_handle),
			width,
			height,
			allow_hdr,
			back_buffer_size));
}
void LCDevice::destroy_swap_chain(uint64 handle) noexcept {
	delete reinterpret_cast<LCSwapChain*>(handle);
}
PixelStorage LCDevice::swap_chain_pixel_storage(uint64 handle) noexcept {
	return PixelStorage::BYTE4;
}
void LCDevice::present_display_in_stream(uint64 stream_handle, uint64 swapchain_handle, uint64 image_handle) noexcept {
	reinterpret_cast<LCCmdBuffer*>(stream_handle)
		->Present(
			reinterpret_cast<LCSwapChain*>(swapchain_handle),
			reinterpret_cast<RenderTexture*>(image_handle));
}
VSTL_EXPORT_C LCDeviceInterface* create(Context const& c, std::string_view) {
	return new LCDevice(c);
}
VSTL_EXPORT_C void destroy(LCDeviceInterface* device) {
	delete static_cast<LCDevice*>(device);
}
}// namespace toolhub::directx