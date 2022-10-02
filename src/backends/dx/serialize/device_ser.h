#pragma once
#include <vstl/Common.h>
#include <runtime/device.h>
#include "array_iostream.h"
#include "ast_serde.h"
#include "socket_visitor.h"
#include <vstl/LockFreeArrayQueue.h>
#include <serialize/DatabaseInclude.h>
namespace luisa::compute {
class DeviceSer : public vstd::IOperatorNewBase, public Device::Interface {
	struct StreamDispatch {
		SocketVisitor::WaitReceive waitFunc;
		luisa::vector<std::pair<void*, size_t>> vec;
		void ExecuteCommand(DeviceSer* self);
	};
	struct Stream : public vstd::IOperatorNewBase {
		std::condition_variable cv;
		std::mutex mtx;
		vstd::LockFreeArrayQueue<
			vstd::variant<StreamDispatch, luisa::move_only_function<void()>>>
			taskQueue;
	};
	std::mutex mtx;
	SocketVisitor* visitor = nullptr;
	vstd::LockFreeArrayQueue<vstd::unique_ptr<AstSerializer>> serializers;
	vstd::LockFreeArrayQueue<vstd::unique_ptr<IJsonDatabase>> dbs;
	std::thread dispatchThread;
	uint64 handleCounter = 0;
	bool threadEnable = true;
	vstd::LockFreeArrayQueue<Stream*> dispatchTasks;
	std::condition_variable dispatchCv;
	std::mutex dispatchMtx;

	uint64 finishedFence = 0;
	vstd::HashMap<uint64> seredFunctionHashes;

public:
	ArrayIStream arr;

	void* native_handle() const noexcept override { return nullptr; }

	// buffer
	uint64_t create_buffer(size_t size_bytes) noexcept override;
	void destroy_buffer(uint64_t handle) noexcept override;
	void* buffer_native_handle(uint64_t handle) const noexcept override { return nullptr; }
	void set_binary_io(BinaryIO* visitor) noexcept override;
	// texture
	uint64_t create_texture(
		PixelFormat format, uint dimension,
		uint width, uint height, uint depth,
		uint mipmap_levels) noexcept override;
	void destroy_texture(uint64_t handle) noexcept override;
	void* texture_native_handle(uint64_t handle) const noexcept override { return nullptr; }

	// bindless array
	uint64_t create_bindless_array(size_t size) noexcept override;
	void destroy_bindless_array(uint64_t handle) noexcept override;
	void emplace_buffer_in_bindless_array(uint64_t array, size_t index, uint64_t handle, size_t offset_bytes) noexcept override;
	void emplace_tex2d_in_bindless_array(uint64_t array, size_t index, uint64_t handle, Sampler sampler) noexcept override;
	void emplace_tex3d_in_bindless_array(uint64_t array, size_t index, uint64_t handle, Sampler sampler) noexcept override;
	void remove_buffer_in_bindless_array(uint64_t array, size_t index) noexcept override;
	void remove_tex2d_in_bindless_array(uint64_t array, size_t index) noexcept override;
	void remove_tex3d_in_bindless_array(uint64_t array, size_t index) noexcept override;
	IUtil* get_util() noexcept override { return nullptr; }
	// stream
	uint64_t create_stream(bool allowPresent) noexcept override;

	void destroy_stream(uint64_t handle) noexcept override;
	void synchronize_stream(uint64_t stream_handle) noexcept override;
	void dispatch(uint64_t stream_handle, CommandList&& list) noexcept override;
	void dispatch(uint64_t stream_handle, luisa::move_only_function<void()>&& func) noexcept override;
	void* stream_native_handle(uint64_t handle) const noexcept override { return nullptr; }

	// kernel
	uint64_t create_shader(Function kernel, vstd::string_view file_name) noexcept override;
	uint64_t load_shader(vstd::string_view file_name, vstd::span<Type const* const> types) noexcept override;
	void destroy_shader(uint64_t handle) noexcept override;

	// event
	uint64_t create_event() noexcept override;
	void destroy_event(uint64_t handle) noexcept override;
	void signal_event(uint64_t handle, uint64_t stream_handle) noexcept override;
	void wait_event(uint64_t handle, uint64_t stream_handle) noexcept override;
	void synchronize_event(uint64_t handle) noexcept override;
	// accel
	uint64_t create_mesh(
		AccelUpdateHint hint,
		bool allow_compact, bool allow_update) noexcept override;
	void destroy_mesh(uint64_t handle) noexcept override;

	uint64_t create_accel(AccelUpdateHint hint, bool allow_compact, bool allow_update) noexcept override;

	void destroy_accel(uint64_t handle) noexcept override;
	// swap chain
	uint64_t create_swap_chain(
		uint64 window_handle,
		uint64 stream_handle,
		uint width,
		uint height,
		bool allow_hdr,
		uint back_buffer_size) noexcept override { return 0; }
	void destroy_swap_chain(uint64_t handle) noexcept override {}
	PixelStorage swap_chain_pixel_storage(uint64_t handle) noexcept override { return PixelStorage::BYTE4; }
	void present_display_in_stream(uint64_t stream_handle, uint64_t swapchain_handle, uint64_t image_handle) noexcept override {}
	DeviceSer(Context ctx);
	~DeviceSer();

private:
	void DispatchThread();
};
}// namespace luisa::compute