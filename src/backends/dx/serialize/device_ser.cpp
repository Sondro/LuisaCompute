#include "device_ser.h"
#include "ser_tag.h"
namespace luisa::compute {
//TODO: process return value
uint64_t DeviceSer::create_buffer(size_t size_bytes) noexcept {
	arr << SerTag::CreateBuffer << size_bytes;
}
void DeviceSer::destroy_buffer(uint64_t handle) noexcept {
	arr << SerTag::DestroyBuffer << handle;
}
void DeviceSer::set_io_visitor(BinaryIOVisitor* visitor) noexcept {
	LUISA_ERROR("Remote device no support customized io-visitor");
}
// texture
uint64_t DeviceSer::create_texture(
	PixelFormat format, uint dimension,
	uint width, uint height, uint depth,
	uint mipmap_levels) noexcept {
	arr << SerTag::CreateTexture << format << dimension << width << height << depth << mipmap_levels;
}
void DeviceSer::destroy_texture(uint64_t handle) noexcept {
	arr << SerTag::DestroyTexture << handle;
}
uint64_t DeviceSer::create_bindless_array(size_t size) noexcept {
	arr << SerTag::CreateBindlessArray << size;
}
void DeviceSer::destroy_bindless_array(uint64_t handle) noexcept {
	arr << SerTag::DestroyBindlessArray << handle;
}
void DeviceSer::emplace_buffer_in_bindless_array(uint64_t array, size_t index, uint64_t handle, size_t offset_bytes) noexcept {
	arr << SerTag::EmplaceBufferInBindless << array << index << handle << offset_bytes;
}
void DeviceSer::emplace_tex2d_in_bindless_array(uint64_t array, size_t index, uint64_t handle, Sampler sampler) noexcept {
	arr << SerTag::EmplaceTex2DInBindless << array << index << handle << reinterpret_cast<vstd::Storage<Sampler>&>(sampler);
}
void DeviceSer::emplace_tex3d_in_bindless_array(uint64_t array, size_t index, uint64_t handle, Sampler sampler) noexcept {
	arr << SerTag::EmplaceTex3DInBindless << array << index << handle << reinterpret_cast<vstd::Storage<Sampler>&>(sampler);
}
void DeviceSer::remove_buffer_in_bindless_array(uint64_t array, size_t index) noexcept {
	arr << SerTag::RemoveBufferInBindless << array << index;
}
void DeviceSer::remove_tex2d_in_bindless_array(uint64_t array, size_t index) noexcept {
	arr << SerTag::RemoveTex2DInBindless << array << index;
}
void DeviceSer::remove_tex3d_in_bindless_array(uint64_t array, size_t index) noexcept {
	arr << SerTag::RemoveTex3DInBindless << array << index;
}
// stream
uint64_t DeviceSer::create_stream(bool allowPresent) noexcept {
	assert(!allowPresent);
	arr << SerTag::CreateStream;
}

void DeviceSer::destroy_stream(uint64_t handle) noexcept {
	arr << SerTag::DestroyStream << handle;
}
void DeviceSer::synchronize_stream(uint64_t stream_handle) noexcept {
    arr << SerTag::SyncStream << stream_handle;
}
void DeviceSer::dispatch(uint64_t stream_handle, CommandList&& list) noexcept {}
void DeviceSer::dispatch(uint64_t stream_handle, luisa::move_only_function<void()>&& func) noexcept {}
uint64_t DeviceSer::create_shader(Function kernel, vstd::string_view file_name) noexcept {}
uint64_t DeviceSer::load_shader(vstd::string_view file_name, vstd::span<Type const* const> types) noexcept {}
void DeviceSer::destroy_shader(uint64_t handle) noexcept {}

// event
uint64_t DeviceSer::create_event() noexcept {}
void DeviceSer::destroy_event(uint64_t handle) noexcept {}
void DeviceSer::signal_event(uint64_t handle, uint64_t stream_handle) noexcept {}
void DeviceSer::wait_event(uint64_t handle, uint64_t stream_handle) noexcept {}
void DeviceSer::synchronize_event(uint64_t handle) noexcept {}
// accel
uint64_t DeviceSer::create_mesh(
	AccelUsageHint hint,
	bool allow_compact, bool allow_update) noexcept {}
void DeviceSer::destroy_mesh(uint64_t handle) noexcept {}

uint64_t DeviceSer::create_accel(AccelUsageHint hint, bool allow_compact, bool allow_update) noexcept {}

void DeviceSer::destroy_accel(uint64_t handle) noexcept {}
}// namespace luisa::compute