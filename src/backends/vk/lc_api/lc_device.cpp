#include "lc_device.h"
#include "lc_stream.h"
#include <gpu_collection/buffer.h>
#include <gpu_collection/texture.h>
#include <gpu_collection/bindless_array.h>
#include <vk_rtx/mesh.h>
#include <vk_runtime/event.h>
#include <vk_rtx/accel.h>
#include <shader/pipeline_cache.h>
#include <shader/compute_shader.h>
#include <shader/rt_shader.h>
namespace toolhub::vk {
namespace detail {
static VkInstance vkInstance = nullptr;
static size_t vkDeviceCount = 0;
static std::mutex vkInstMtx;
}// namespace detail
VkFormat LCDevice::GetNativeFormat(PixelFormat pixelFormat) {
	switch (pixelFormat) {
		case PixelFormat::R8SInt:
			return VK_FORMAT_R8_SINT;
		case PixelFormat::R8UInt:
			return VK_FORMAT_R8_UINT;
		case PixelFormat::R8UNorm:
			return VK_FORMAT_R8_UNORM;
		case PixelFormat::RG8SInt:
			return VK_FORMAT_R8_SINT;
		case PixelFormat::RG8UInt:
			return VK_FORMAT_R8_UINT;
		case PixelFormat::RG8UNorm:
			return VK_FORMAT_R8G8_UNORM;
		case PixelFormat::RGBA8SInt:
			return VK_FORMAT_R8G8B8A8_SINT;
		case PixelFormat::RGBA8UInt:
			return VK_FORMAT_R8G8B8A8_UINT;
		case PixelFormat::RGBA8UNorm:
			return VK_FORMAT_R8G8B8A8_UNORM;

		case PixelFormat::R16SInt:
			return VK_FORMAT_R16_SINT;
		case PixelFormat::R16UInt:
			return VK_FORMAT_R16_UINT;
		case PixelFormat::R16UNorm:
			return VK_FORMAT_R16_UNORM;
		case PixelFormat::RG16SInt:
			return VK_FORMAT_R16G16_SINT;
		case PixelFormat::RG16UInt:
			return VK_FORMAT_R16G16_UINT;
		case PixelFormat::RG16UNorm:
			return VK_FORMAT_R16G16_UNORM;
		case PixelFormat::RGBA16SInt:
			return VK_FORMAT_R16G16B16A16_SINT;
		case PixelFormat::RGBA16UInt:
			return VK_FORMAT_R16G16B16A16_UINT;
		case PixelFormat::RGBA16UNorm:
			return VK_FORMAT_R16G16B16A16_UNORM;
		case PixelFormat::R32SInt:
			return VK_FORMAT_R32_SINT;
		case PixelFormat::R32UInt:
			return VK_FORMAT_R32_UINT;
		case PixelFormat::RG32SInt:
			return VK_FORMAT_R32G32_SINT;
		case PixelFormat::RG32UInt:
			return VK_FORMAT_R32G32_UINT;

		case PixelFormat::RGBA32SInt:
			return VK_FORMAT_R32G32B32A32_SINT;
		case PixelFormat::RGBA32UInt:
			return VK_FORMAT_R32G32B32A32_UINT;
		case PixelFormat::R16F:
			return VK_FORMAT_R16_SFLOAT;
		case PixelFormat::RG16F:
			return VK_FORMAT_R16G16_SFLOAT;
		case PixelFormat::RGBA16F:
			return VK_FORMAT_R16G16B16A16_SFLOAT;

		case PixelFormat::R32F:
			return VK_FORMAT_R32_SFLOAT;
		case PixelFormat::RG32F:
			return VK_FORMAT_R32G32_SFLOAT;
		case PixelFormat::RGBA32F:
			return VK_FORMAT_R32G32B32A32_SFLOAT;

		case PixelFormat::BC4UNorm:
			return VK_FORMAT_BC4_UNORM_BLOCK;
		case PixelFormat::BC5UNorm:
			return VK_FORMAT_BC5_UNORM_BLOCK;
		case PixelFormat::BC6HUF16:
			return VK_FORMAT_BC6H_UFLOAT_BLOCK;
		case PixelFormat::BC7UNorm:
			return VK_FORMAT_BC7_UNORM_BLOCK;
		default:
			return VK_FORMAT_UNDEFINED;
	}
}

LCDevice::LCDevice(Context const& ctx)
	: luisa::compute::Device::Interface(ctx) {
	{
		std::lock_guard lck(detail::vkInstMtx);
		if (detail::vkInstance == nullptr) {
			detail::vkInstance = Device::InitVkInstance();
		}
		detail::vkDeviceCount++;
	}
	device = Device::CreateDevice(detail::vkInstance, nullptr, 0);
}
LCDevice::~LCDevice() {
	delete device;
	{
		std::lock_guard lck(detail::vkInstMtx);
		if (--detail::vkDeviceCount == 0) {
			vkDestroyInstance(
				detail::vkInstance,
				Device::Allocator());
		}
	}
}
void* LCDevice::native_handle() const noexcept {
	return device->device;
}

// buffer
uint64_t LCDevice::create_buffer(size_t size_bytes) noexcept {
	return reinterpret_cast<uint64>(new Buffer(
		device,
		size_bytes,
		false,
		RWState::None));
}
void LCDevice::destroy_buffer(uint64_t handle) noexcept {
	delete reinterpret_cast<Buffer*>(handle);
}
void* LCDevice::buffer_native_handle(uint64_t handle) const noexcept {
	return reinterpret_cast<Buffer*>(handle)->GetResource();
}

// texture
uint64_t LCDevice::create_texture(
	PixelFormat format, uint dimension,
	uint width, uint height, uint depth,
	uint mipmap_levels) noexcept {
	return reinterpret_cast<uint64>(new Texture(
		device,
		uint3(width, height, depth),
		GetNativeFormat(format),
		(VkImageType)(dimension - 1),
		mipmap_levels));
}
void LCDevice::destroy_texture(uint64_t handle) noexcept {
	delete reinterpret_cast<Texture*>(handle);
}
void* LCDevice::texture_native_handle(uint64_t handle) const noexcept {
	return reinterpret_cast<Texture*>(handle)->GetResource();
}

// bindless array
uint64_t LCDevice::create_bindless_array(size_t size) noexcept {
	return reinterpret_cast<uint64>(new BindlessArray(
		device,
		size));
}
void LCDevice::destroy_bindless_array(uint64_t handle) noexcept {
	auto arr = reinterpret_cast<BindlessArray*>(handle);
	delete arr;
}
void LCDevice::emplace_buffer_in_bindless_array(uint64_t array, size_t index, uint64_t handle, size_t offset_bytes) noexcept {
	auto arr = reinterpret_cast<BindlessArray*>(array);
	arr->Bind(
		index,
		reinterpret_cast<Buffer*>(handle),
		offset_bytes);
}
void LCDevice::emplace_tex2d_in_bindless_array(uint64_t array, size_t index, uint64_t handle, Sampler sampler) noexcept {
	auto arr = reinterpret_cast<BindlessArray*>(array);
	//TODO: sampler currently not supported
	// should re-design bindless sampler
	arr->Bind(
		index,
		reinterpret_cast<Texture*>(handle),
		0);
}
void LCDevice::emplace_tex3d_in_bindless_array(uint64_t array, size_t index, uint64_t handle, Sampler sampler) noexcept {
	emplace_tex2d_in_bindless_array(array, index, handle, sampler);
}
void LCDevice::remove_buffer_in_bindless_array(uint64_t array, size_t index) noexcept {
	auto arr = reinterpret_cast<BindlessArray*>(array);
	arr->UnBindBuffer(index);
}
void LCDevice::remove_tex2d_in_bindless_array(uint64_t array, size_t index) noexcept {
	auto arr = reinterpret_cast<BindlessArray*>(array);
	arr->UnBindTex2D(index);
}
void LCDevice::remove_tex3d_in_bindless_array(uint64_t array, size_t index) noexcept {
	auto arr = reinterpret_cast<BindlessArray*>(array);
	arr->UnBindTex3D(index);
}

// stream
uint64_t LCDevice::create_stream(bool for_present) noexcept {
	return reinterpret_cast<uint64>(new LCStream(
		device));
}
void LCDevice::destroy_stream(uint64_t handle) noexcept {
	delete reinterpret_cast<LCStream*>(handle);
}
void LCDevice::synchronize_stream(uint64_t stream_handle) noexcept {
	reinterpret_cast<LCStream*>(stream_handle)->Complete();
}
void LCDevice::dispatch(uint64_t stream_handle, CommandList&& list) noexcept {
	reinterpret_cast<LCStream*>(stream_handle)->DispatchCmd(std::move(list));
}
//TODO: need re-design
/* void LCDevice::dispatch(uint64_t stream_handle, luisa::span<const CommandList> lists) noexcept {
            for (auto &&list : lists) { dispatch(stream_handle, list); }
        }*/
void LCDevice::dispatch(uint64_t stream_handle, luisa::move_only_function<void()>&& func) noexcept {
	reinterpret_cast<LCStream*>(stream_handle)->AddCallback(std::move(func));
}
void* LCDevice::stream_native_handle(uint64_t handle) const noexcept {
	reinterpret_cast<LCStream*>(handle)->Pool();
}
// swap chain
uint64_t LCDevice::create_swap_chain(
	uint64_t window_handle, uint64_t stream_handle, uint width, uint height,
	bool allow_hdr, uint back_buffer_size) noexcept {
	//TODO
	return 0;
}
void LCDevice::destroy_swap_chain(uint64_t handle) noexcept {
	//TODO
}
PixelStorage LCDevice::swap_chain_pixel_storage(uint64_t handle) noexcept {
	return PixelStorage::BYTE4;
}
void LCDevice::present_display_in_stream(uint64_t stream_handle, uint64_t swapchain_handle, uint64_t image_handle) noexcept {
	//TODO
}
// kernel
uint64_t LCDevice::create_shader(Function kernel, std::string_view meta_options) noexcept {
	//Debug now
	vstd::small_vector<VkDescriptorType> types;
	types.emplace_back(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	for (auto&& i : kernel.arguments()) {
		auto isWrite = (static_cast<uint>(kernel.variable_usage(i.uid())) & static_cast<uint>(Usage::WRITE)) != 0;
		using Tag = luisa::compute::Variable::Tag;
		switch (i.tag()) {
			case Tag::BUFFER:
				types.emplace_back(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
				break;
			case Tag::TEXTURE:
				if (isWrite) {
					types.emplace_back(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
				} else {
					types.emplace_back(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
				}
				break;
			case Tag::BINDLESS_ARRAY:
				types.emplace_back(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

				break;
			case Tag::ACCEL:
				if (isWrite) {
					types.emplace_back(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
				} else {
					types.emplace_back(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
				}
				break;
		}
	}
	auto printErrCode = [&](vstd::string const& errCode) {
		std::cout << "compile error: " << errCode << '\n';
		assert(false);
		return nullptr;
	};
	//Test raytracing
	//if(kernel.raytracing())
	auto Cast = [](auto&& value) {
		return reinterpret_cast<uint64>(static_cast<IShader*>(value));
	};
	if (true) {
		auto raygenResult = compiler.CompileSpirV(kernel, true, false);
		//auto raygenResult = compiler.CompileExistsFile(kernel, true, false, "rt_spvasm/raygen_output.spvasm");
		auto missResult = compiler.CompileExistsFile(kernel, true, false, "rt_spvasm/miss_output.spvasm");
		auto closestResult = compiler.CompileExistsFile(kernel, true, false, "rt_spvasm/closest_output.spvasm");
		auto Check = [&](auto&& result) {
			if (auto v = result.try_get<vstd::string>()) {
				printErrCode(*v);
				assert(false);
			}
		};
		Check(raygenResult);
		Check(missResult);
		Check(closestResult);
		auto value = new RTShader(
			device,
			raygenResult.get<0>(),
			missResult.get<0>(),
			closestResult.get<0>(),
			nullptr,
			types);
		return Cast(value);
	} else {
		auto compileResult = compiler.CompileSpirV(kernel, true, false);
		auto value = compileResult.multi_visit_or(
			vstd::UndefEval<ComputeShader*>{},
			[&](spvstd::vector<uint> const& code) {
				auto cs = new ComputeShader(
					device,
					code,
					nullptr,
					types,
					kernel.block_size());
				return cs;
			},
			printErrCode);
		return Cast(value);
	}
}
void LCDevice::destroy_shader(uint64_t handle) noexcept {
	delete reinterpret_cast<IShader*>(handle);
}

// event
uint64_t LCDevice::create_event() noexcept {
	return reinterpret_cast<uint64>(new Event(device));
}
void LCDevice::destroy_event(uint64_t handle) noexcept {
	delete reinterpret_cast<Event*>(handle);
}
void LCDevice::signal_event(uint64_t handle, uint64_t stream_handle) noexcept {
	reinterpret_cast<LCStream*>(stream_handle)->PreparingFrame()->SignalSemaphore(reinterpret_cast<Event*>(handle)->Semaphore());
}
void LCDevice::wait_event(uint64_t handle, uint64_t stream_handle) noexcept {
	reinterpret_cast<LCStream*>(stream_handle)->PreparingFrame()->WaitSemaphore(reinterpret_cast<Event*>(handle)->Semaphore());
}
void LCDevice::synchronize_event(uint64_t handle) noexcept {
	reinterpret_cast<Event*>(handle)->Wait();
}

// accel
uint64_t LCDevice::create_mesh(
	AccelUsageHint hint,
	bool allow_compact, bool allow_update) noexcept {
	return reinterpret_cast<uint64>(new Mesh(
		device,
		allow_update,
		allow_compact,
		hint == AccelUsageHint::FAST_TRACE));
}
void LCDevice::destroy_mesh(uint64_t handle) noexcept {
	delete reinterpret_cast<Mesh*>(handle);
}
uint64_t LCDevice::create_accel(AccelUsageHint hint, bool allow_compact, bool allow_update) noexcept {
	return reinterpret_cast<uint64>(new Accel(
		device,
		allow_update,
		allow_compact,
		hint == AccelUsageHint::FAST_TRACE));
}
void LCDevice::destroy_accel(uint64_t handle) noexcept {
	delete reinterpret_cast<Accel*>(handle);
}
}// namespace toolhub::vk