#pragma once
#include <runtime/device.h>
#include <components/device.h>
#include <spv/compile/spv_compiler.h>
using namespace luisa;
using namespace luisa::compute;
namespace toolhub::vk {
class LCDevice : public luisa::compute::Device::Interface {
	Device* device;
	spv::SpvCompiler compiler;

public:
	static VkFormat GetNativeFormat(PixelFormat pixelFormat);
	LCDevice(Context const& ctx);
	~LCDevice();
	void* native_handle() const noexcept override;

	// buffer
	uint64_t create_buffer(size_t size_bytes) noexcept override;
	void destroy_buffer(uint64_t handle) noexcept override;
	void* buffer_native_handle(uint64_t handle) const noexcept override;

	// texture
	uint64_t create_texture(
		PixelFormat format, uint dimension,
		uint width, uint height, uint depth,
		uint mipmap_levels) noexcept override;
	void destroy_texture(uint64_t handle) noexcept override;
	void* texture_native_handle(uint64_t handle) const noexcept override;

	// bindless array
	uint64_t create_bindless_array(size_t size) noexcept override;
	void destroy_bindless_array(uint64_t handle) noexcept override;
	void emplace_buffer_in_bindless_array(uint64_t array, size_t index, uint64_t handle, size_t offset_bytes) noexcept override;
	void emplace_tex2d_in_bindless_array(uint64_t array, size_t index, uint64_t handle, Sampler sampler) noexcept override;
	void emplace_tex3d_in_bindless_array(uint64_t array, size_t index, uint64_t handle, Sampler sampler) noexcept override;
	void remove_buffer_in_bindless_array(uint64_t array, size_t index) noexcept override;
	void remove_tex2d_in_bindless_array(uint64_t array, size_t index) noexcept override;
	void remove_tex3d_in_bindless_array(uint64_t array, size_t index) noexcept override;

	// stream
	uint64_t create_stream(bool for_present) noexcept override;
	void destroy_stream(uint64_t handle) noexcept override;
	void synchronize_stream(uint64_t stream_handle) noexcept override;
	void dispatch(uint64_t stream_handle, CommandList&& list) noexcept override;
	//TODO: need re-design
	/* void dispatch(uint64_t stream_handle, luisa::span<const CommandList> lists) noexcept {
            for (auto &&list : lists) { dispatch(stream_handle, list); }
        }*/
	void dispatch(uint64_t stream_handle, luisa::move_only_function<void()>&& func) noexcept override;
	void* stream_native_handle(uint64_t handle) const noexcept override;
	// swap chain
	uint64_t create_swap_chain(
		uint64_t window_handle, uint64_t stream_handle, uint width, uint height,
		bool allow_hdr, uint back_buffer_size) noexcept override;
	void destroy_swap_chain(uint64_t handle) noexcept override;
	PixelStorage swap_chain_pixel_storage(uint64_t handle) noexcept override;
	void present_display_in_stream(uint64_t stream_handle, uint64_t swapchain_handle, uint64_t image_handle) noexcept override;
	// kernel
	uint64_t create_shader(Function kernel, std::string_view meta_options) noexcept override;
	void destroy_shader(uint64_t handle) noexcept override;

	// event
	uint64_t create_event() noexcept override;
	void destroy_event(uint64_t handle) noexcept override;
	void signal_event(uint64_t handle, uint64_t stream_handle) noexcept override;
	void wait_event(uint64_t handle, uint64_t stream_handle) noexcept override;
	void synchronize_event(uint64_t handle) noexcept override;

	// accel
	uint64_t create_mesh(
		AccelUsageHint hint,
		bool allow_compact, bool allow_update) noexcept override;
	void destroy_mesh(uint64_t handle) noexcept override;
	uint64_t create_accel(AccelUsageHint hint, bool allow_compact, bool allow_update) noexcept override;
	void destroy_accel(uint64_t handle) noexcept override;

	// query
	//IUtil* get_util() override { return nullptr; }
};
}// namespace toolhub::vk