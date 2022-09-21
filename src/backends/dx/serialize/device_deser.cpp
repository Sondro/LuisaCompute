#include "device_deser.h"
#include "ser_tag.h"
#include "ast_serde.h"
namespace luisa::compute {
#define DEVICEDESER_DESTROY(func)          \
	{                                      \
		std::lock_guard lck(handleMtx);    \
		uint64 handle;                     \
		strm >> handle;                    \
		auto ite = handleMap.Find(handle); \
		assert(ite);                       \
		nativeDevice->func(ite.Value());   \
		handleMap.Remove(ite);             \
	}

uint64 DeviceDeser::HandleFilter(uint64 handle) const {
	std::shared_lock lck(handleMtx);
	auto ite = handleMap.Find(handle);
	assert(ite);
	return ite.Value();
}
void DeviceDeser::Receive(SocketVisitor* visitor) {
	dataArr.clear();
	visitor->receive(dataArr);
	ArrayOStream strm;
	strm.ptr = dataArr.data();
	strm.end = dataArr.data() + dataArr.size();
	SerTag serTag;
	switch (serTag) {
		case SerTag::CreateDevice: {
			nativeDevice = createDeviceFunc();
		} break;
		case SerTag::DestroyDevice: {
			destroyDeviceFunc(nativeDevice);
			nativeDevice = nullptr;
		} break;
		case SerTag::CreateBuffer: {
			uint64 handle;
			size_t size;
			strm >> handle >> size;
			auto nativeHandle = nativeDevice->create_buffer(size);
			std::lock_guard lck(handleMtx);
			handleMap.Emplace(handle, nativeHandle);
		} break;
		case SerTag::DestroyBuffer: {
			DEVICEDESER_DESTROY(destroy_buffer)
		} break;
		case SerTag::CreateTexture: {
			PixelFormat format;
			uint dimension;
			uint width;
			uint height;
			uint depth;
			uint mipmap_levels;
			uint64 handle;
			strm >> handle >> format >> dimension >> width >> height >> depth >> mipmap_levels;
			auto nativeHandle = nativeDevice->create_texture(format, dimension, width, height, depth, mipmap_levels);
			std::lock_guard lck(handleMtx);
			handleMap.Emplace(handle, nativeHandle);
		} break;
		case SerTag::DestroyTexture: {
			DEVICEDESER_DESTROY(destroy_texture)
		} break;
		case SerTag::CreateBindlessArray: {
			uint64 handle;
			size_t size;
			strm >> handle >> size;
			auto nativeHandle = nativeDevice->create_bindless_array(size);
			std::lock_guard lck(handleMtx);
			handleMap.Emplace(handle, nativeHandle);
		} break;
		case SerTag::DestroyBindlessArray: {
			DEVICEDESER_DESTROY(destroy_bindless_array)
		} break;
		case SerTag::EmplaceBufferInBindless: {
			uint64_t array;
			size_t index;
			uint64_t handle;
			size_t offset_bytes;
			strm >> array >> index >> handle >> offset_bytes;
			array = HandleFilter(array);
			handle = HandleFilter(handle);
			nativeDevice->emplace_buffer_in_bindless_array(array, index, handle, offset_bytes);
		} break;
		case SerTag::EmplaceTex2DInBindless: {
			uint64_t array;
			size_t index;
			uint64_t handle;
			Sampler sampler;
			strm >> array >> index >> handle >> reinterpret_cast<vstd::Storage<Sampler>&>(sampler);
			array = HandleFilter(array);
			handle = HandleFilter(handle);
			nativeDevice->emplace_tex2d_in_bindless_array(array, index, handle, sampler);
		} break;
		case SerTag::EmplaceTex3DInBindless: {
			uint64_t array;
			size_t index;
			uint64_t handle;
			Sampler sampler;
			strm >> array >> index >> handle >> reinterpret_cast<vstd::Storage<Sampler>&>(sampler);
			array = HandleFilter(array);
			handle = HandleFilter(handle);
			nativeDevice->emplace_tex3d_in_bindless_array(array, index, handle, sampler);
		} break;
		case SerTag::RemoveBufferInBindless: {
			uint64 array;
			size_t index;
			strm >> array >> index;
			array = HandleFilter(array);
			nativeDevice->remove_buffer_in_bindless_array(array, index);
		} break;
		case SerTag::RemoveTex2DInBindless: {
			uint64 array;
			size_t index;
			strm >> array >> index;
			array = HandleFilter(array);
			nativeDevice->remove_tex2d_in_bindless_array(array, index);
		} break;
		case SerTag::RemoveTex3DInBindless: {
			uint64 array;
			size_t index;
			strm >> array >> index;
			array = HandleFilter(array);
			nativeDevice->remove_tex3d_in_bindless_array(array, index);
		} break;
		case SerTag::CreateStream: {
			uint64 handle;
			strm >> handle;
			auto nativeHandle = nativeDevice->create_stream(false);
			vstd::optional<RemoteStream>* stream;
			{
				std::lock_guard lck(handleMtx);
				stream = &streamMap.Emplace(handle).Value();
			}
			stream->New(nativeDevice, this, nativeHandle);
		} break;
		case SerTag::DestroyStream: {
			uint64 handle;
			strm >> handle;
			std::lock_guard lck(handleMtx);
			auto ite = streamMap.Find(handle);
			assert(ite);
			streamMap.Remove(ite);
		} break;
		case SerTag::SyncStream: {

		} break;
		case SerTag::DispatchCommandList: {
		} break;
		case SerTag::CreateShader: {
		} break;
		case SerTag::DestroyShader: {
			DEVICEDESER_DESTROY(destroy_shader)
		} break;
		case SerTag::LoadShader: {
		} break;
		case SerTag::CreateGpuEvent: {
		} break;
		case SerTag::DestroyGpuEvent: {
			DEVICEDESER_DESTROY(destroy_event)
		} break;
		case SerTag::SignalGpuEvent: {
		} break;
		case SerTag::WaitGpuEvent: {
		} break;
		case SerTag::SyncGpuEvent: {
		} break;
		case SerTag::CreateMesh: {
		} break;
		case SerTag::DestroyMesh: {
			DEVICEDESER_DESTROY(destroy_mesh)
		} break;
		case SerTag::CreateAccel: {
		} break;
		case SerTag::DestroyAccel: {
			DEVICEDESER_DESTROY(destroy_accel)
		} break;
		case SerTag::TransportFunction: {
			uint64 hash;
			strm >> hash;
			AstSerializer ser{};
			auto db = vstd::create_unique(CreateDatabase());
			detail::FunctionBuilder* builder;
			{
				std::lock_guard lck(handleMtx);
				builder = &funcMap.Emplace(hash, Function::Tag::KERNEL).Value();
			}
			ser.DeserializeKernel(builder, db->GetRootNode());
			break;
		}
	}
}

RemoteStream::RemoteStream(Device::Interface* device, DeviceDeser* deserDevice, uint64 nativeHandle)
	: thd([this] { ThreadLogic(); }), device(device) {
	visitor.deserDevice = deserDevice;
	cmdDeser.visitor = &visitor;
}
void RemoteStream::ThreadLogic() {
	while (auto v = dstQueue.Pop()) {
		ArrayOStream strm;
		strm.ptr = v->vec.data() + v->startIndex;
		strm.end = v->vec.data() + v->vec.size();
		CommandList list;
		cmdDeser.arr = &strm;
		cmdDeser.DeserCommands(list);
		device->dispatch(nativeHandle, std::move(list));
		executedFrame++;
	}
	std::unique_lock lck(mtx);
	while (dstQueue.Length() == 0) {
		thdCv.wait(lck);
	}
}
uint64 RemoteStream::DeviceSearchVisitor::GetResource(uint64 handle) {
	std::shared_lock lck(deserDevice->handleMtx);
	auto ite = deserDevice->handleMap.Find(handle);
	assert(ite);
	return ite.Value();
}
Function RemoteStream::DeviceSearchVisitor::GetFunction(uint64 handle) {
	std::shared_lock lck(deserDevice->handleMtx);
	auto ite = deserDevice->funcMap.Find(handle);
	assert(ite);
	return Function(&ite.Value());
}
RemoteStream::~RemoteStream() {}
#undef DEVICEDESER_DESTROY
}// namespace luisa::compute