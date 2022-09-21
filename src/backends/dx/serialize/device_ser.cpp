#include "device_ser.h"
#include "ser_tag.h"
#include "cmd_ser.h"
namespace luisa::compute {
//TODO: process return value
uint64_t DeviceSer::create_buffer(size_t size_bytes) noexcept {
	std::lock_guard lck(mtx);
	auto handle = handleCounter++;
	arr << SerTag::CreateBuffer << handle << size_bytes;
	visitor->send_async(arr.steal());
	return handle;
}
void DeviceSer::destroy_buffer(uint64_t handle) noexcept {
	std::lock_guard lck(mtx);
	arr << SerTag::DestroyBuffer << handle;
	visitor->send_async(arr.steal());
}
void DeviceSer::set_io_visitor(BinaryIOVisitor* visitor) noexcept {
	LUISA_ERROR("Remote device no support customized io-visitor");
}
// texture
uint64_t DeviceSer::create_texture(
	PixelFormat format, uint dimension,
	uint width, uint height, uint depth,
	uint mipmap_levels) noexcept {
	std::lock_guard lck(mtx);
	auto handle = handleCounter++;
	arr << SerTag::CreateTexture << handle << format << dimension << width << height << depth << mipmap_levels;
	visitor->send_async(arr.steal());
	return handle;
}
void DeviceSer::destroy_texture(uint64_t handle) noexcept {
	std::lock_guard lck(mtx);
	arr << SerTag::DestroyTexture << handle;
	visitor->send_async(arr.steal());
}
uint64_t DeviceSer::create_bindless_array(size_t size) noexcept {
	std::lock_guard lck(mtx);
	auto handle = handleCounter++;
	arr << SerTag::CreateBindlessArray << handle << size;
	visitor->send_async(arr.steal());
	return handle;
}
void DeviceSer::destroy_bindless_array(uint64_t handle) noexcept {
	std::lock_guard lck(mtx);
	arr << SerTag::DestroyBindlessArray << handle;
	visitor->send_async(arr.steal());
}
void DeviceSer::emplace_buffer_in_bindless_array(uint64_t array, size_t index, uint64_t handle, size_t offset_bytes) noexcept {
	std::lock_guard lck(mtx);
	arr << SerTag::EmplaceBufferInBindless << array << index << handle << offset_bytes;
	visitor->send_async(arr.steal());
}
void DeviceSer::emplace_tex2d_in_bindless_array(uint64_t array, size_t index, uint64_t handle, Sampler sampler) noexcept {
	std::lock_guard lck(mtx);
	arr << SerTag::EmplaceTex2DInBindless << array << index << handle << reinterpret_cast<vstd::Storage<Sampler>&>(sampler);
	visitor->send_async(arr.steal());
}
void DeviceSer::emplace_tex3d_in_bindless_array(uint64_t array, size_t index, uint64_t handle, Sampler sampler) noexcept {
	std::lock_guard lck(mtx);
	arr << SerTag::EmplaceTex3DInBindless << array << index << handle << reinterpret_cast<vstd::Storage<Sampler>&>(sampler);
	visitor->send_async(arr.steal());
}
void DeviceSer::remove_buffer_in_bindless_array(uint64_t array, size_t index) noexcept {
	std::lock_guard lck(mtx);
	arr << SerTag::RemoveBufferInBindless << array << index;
	visitor->send_async(arr.steal());
}
void DeviceSer::remove_tex2d_in_bindless_array(uint64_t array, size_t index) noexcept {
	std::lock_guard lck(mtx);
	arr << SerTag::RemoveTex2DInBindless << array << index;
	visitor->send_async(arr.steal());
}
void DeviceSer::remove_tex3d_in_bindless_array(uint64_t array, size_t index) noexcept {
	std::lock_guard lck(mtx);
	arr << SerTag::RemoveTex3DInBindless << array << index;
	visitor->send_async(arr.steal());
}
// stream
uint64_t DeviceSer::create_stream(bool allowPresent) noexcept {
	assert(!allowPresent);
	auto strm = new Stream();
	auto handle = reinterpret_cast<uint64>(strm);
	{
		std::lock_guard lck(mtx);
		arr << SerTag::CreateStream << handle;
		visitor->send_async(arr.steal());
	}
	return handle;
}

void DeviceSer::destroy_stream(uint64_t handle) noexcept {
	{
		std::lock_guard lck(mtx);
		arr << SerTag::DestroyStream << handle;
		visitor->send_async(arr.steal());
	}
	delete reinterpret_cast<Stream*>(handle);
}
void DeviceSer::synchronize_stream(uint64_t stream_handle) noexcept {
	auto stream = reinterpret_cast<Stream*>(stream_handle);
	std::unique_lock lck(stream->mtx);
	while (stream->taskQueue.Length() > 0) {
		stream->cv.wait(lck);
	}
}
void DeviceSer::dispatch(uint64_t stream_handle, CommandList&& list) noexcept {
	CmdSer cmdSer;
	{
		std::lock_guard lck(mtx);
		cmdSer.arr = &arr;
		arr << SerTag::DispatchCommandList << stream_handle;
		cmdSer.SerCommands(list);
	}
	dispatchTasks.Push(reinterpret_cast<Stream*>(stream_handle));
	dispatchMtx.lock();
	dispatchMtx.unlock();
	dispatchCv.notify_one();
}
void DeviceSer::dispatch(uint64_t stream_handle, luisa::move_only_function<void()>&& func) noexcept {
	auto stream = reinterpret_cast<Stream*>(stream_handle);
	stream->taskQueue.Push(std::move(func));
	dispatchTasks.Push(stream);
	dispatchMtx.lock();
	dispatchMtx.unlock();
	dispatchCv.notify_one();
}
uint64_t DeviceSer::create_shader(Function kernel, vstd::string_view file_name) noexcept {
	auto ite = seredFunctionHashes.TryEmplace(kernel.hash());
	if (ite.second) {
		std::lock_guard lck(mtx);
		arr << SerTag::TransportFunction;
		auto astSerPtr = serializers.Pop();
		auto dbPtr = dbs.Pop();
		if (!astSerPtr) {
			astSerPtr.New(new AstSerializer());
		}
		if (!dbPtr) {
			dbPtr.New(CreateDatabase());
		}
		auto& astSer = *astSerPtr;
		auto& db = *dbPtr;
		auto returnTask = vstd::create_disposer([&] {
			serializers.Push(std::move(astSer));
			dbs.Push(std::move(db));
		});
		db->GetRootNode()->Reset();
		astSer->SerializeKernel(
			kernel,
			db.get());
		db->GetRootNode()->Serialize(
			arr.bytes);
		visitor->send_async(arr.steal());
	}
	size_t handle;
	{
		std::lock_guard lck(mtx);
		handle = handleCounter++;
		arr
			<< SerTag::CreateShader
			<< handle << kernel.hash();
		if (!file_name.empty()) {
			arr << file_name.size()
				<< vstd::span<std::byte const>(reinterpret_cast<std::byte const*>(file_name.data()), file_name.size());
		}
		visitor->send_async(arr.steal());
	}
	return handle;
}
uint64_t DeviceSer::load_shader(vstd::string_view file_name, vstd::span<Type const* const> types) noexcept {
	std::lock_guard lck(mtx);
	auto handle = handleCounter++;
	arr
		<< SerTag::LoadShader
		<< handle
		<< file_name.size()
		<< vstd::span<std::byte const>(reinterpret_cast<std::byte const*>(file_name.data()), file_name.size());
	visitor->send_async(arr.steal());
	return handle;
}
void DeviceSer::destroy_shader(uint64_t handle) noexcept {
	std::lock_guard lck(mtx);
	arr << SerTag::DestroyShader << handle;
	visitor->send_async(arr.steal());
}

// event
uint64_t DeviceSer::create_event() noexcept {
	auto evt = new Event();
	uint64 handle;
	{
		std::lock_guard lck(mtx);
		handle = reinterpret_cast<uint64>(evt);
		arr << SerTag::CreateGpuEvent << handle;
		visitor->send_async(arr.steal());
	}

	return handle;
}
void DeviceSer::destroy_event(uint64_t handle) noexcept {
	{
		std::lock_guard lck(mtx);
		arr << SerTag::DestroyGpuEvent << handle;
		visitor->send_async(arr.steal());
	}
	delete reinterpret_cast<Event*>(handle);
}
void DeviceSer::signal_event(uint64_t handle, uint64_t stream_handle) noexcept {
	auto evt = reinterpret_cast<Event*>(handle);
	{
		std::lock_guard lck(mtx);
		arr << SerTag::SignalGpuEvent << handle << stream_handle;
		visitor->send_async(arr.steal());
		evt->waitFunc = visitor->receive_async();
		evt->count++;
	}
	dispatchTasks.Push(evt);
	{
		dispatchMtx.lock();
		dispatchMtx.unlock();
	}
	dispatchCv.notify_one();
}
void DeviceSer::wait_event(uint64_t handle, uint64_t stream_handle) noexcept {
	std::lock_guard lck(mtx);
	arr << SerTag::WaitGpuEvent << handle << stream_handle;
	visitor->send_async(arr.steal());
}
void DeviceSer::synchronize_event(uint64_t handle) noexcept {
	auto evt = reinterpret_cast<Event*>(handle);
	auto lck = std::unique_lock(evt->mtx);
	while (evt->count > 0) {
		evt->cv.wait(lck);
	}
}

// accel
uint64_t DeviceSer::create_mesh(
	AccelUsageHint hint,
	bool allow_compact, bool allow_update) noexcept {
	std::lock_guard lck(mtx);
	auto handle = handleCounter++;
	vbyte mask = 0;
	if (allow_compact) {
		mask |= 1;
	}
	mask <<= 1;
	if (allow_update) {
		mask |= 1;
	}
	arr << SerTag::CreateMesh << handle << hint << mask;
	visitor->send_async(arr.steal());
	return handle;
}
void DeviceSer::destroy_mesh(uint64_t handle) noexcept {
	std::lock_guard lck(mtx);
	arr << SerTag::DestroyMesh << handle;
	visitor->send_async(arr.steal());
}

uint64_t DeviceSer::create_accel(AccelUsageHint hint, bool allow_compact, bool allow_update) noexcept {
	std::lock_guard lck(mtx);
	auto handle = handleCounter++;
	vbyte mask = 0;
	if (allow_compact) {
		mask |= 1;
	}
	mask <<= 1;
	if (allow_update) {
		mask |= 1;
	}
	arr << SerTag::CreateAccel << handle << hint << mask;
	visitor->send_async(arr.steal());
	return handle;
}

void DeviceSer::destroy_accel(uint64_t handle) noexcept {
	std::lock_guard lck(mtx);
	arr << SerTag::DestroyAccel << handle;
	visitor->send_async(arr.steal());
}
DeviceSer::DeviceSer(Context ctx)
	: Device::Interface(ctx),
	  dispatchThread([this] { DispatchThread(); }) {
}
DeviceSer::~DeviceSer() {
	{
		std::lock_guard lck(dispatchMtx);
		threadEnable = false;
	}
	dispatchCv.notify_one();
	dispatchThread.join();
}

void DeviceSer::StreamDispatch::ExecuteCommand(DeviceSer* self) {
	size_t receiveSize = 0;
	for (auto&& i : vec) {
		receiveSize += i.second;
	}
	auto result = waitFunc();
	assert(result.size() == receiveSize);
	auto ptr = result.data();
	for (auto&& i : vec) {
		memcpy(i.first, ptr, i.second);
		ptr += i.second;
	}
}
void DeviceSer::DispatchThread() {
	while (threadEnable) {
		while (auto v = dispatchTasks.Pop()) {
			v->multi_visit(
				[&](Event* evt) {
					auto vec = evt->waitFunc();
					std::unique_lock lck{evt->mtx};
					if (--evt->count == 0) {
						lck.unlock();
						evt->cv.notify_all();
					}
				},
				[&](Stream* strm) {
					while (auto task = strm->taskQueue.Pop()) {
						task->multi_visit(
							[&](StreamDispatch& dispatch) {
								dispatch.ExecuteCommand(this);
							},
							[&](luisa::move_only_function<void()> const& func) {
								func();
							});
					}
				});
		}
		std::unique_lock lck(dispatchMtx);
		while (dispatchTasks.Length() == 0) {
			dispatchCv.wait(lck);
		}
	}
}
}// namespace luisa::compute