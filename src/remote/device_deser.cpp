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
	auto GetStr = [&](auto strLen) {
		if (strLen > 0) {
			vstd::string str;
			str.resize(strLen);
			return str;
		} else {
			return vstd::string();
		}
	};
	socket = visitor;
	dataArr.clear();
	{
		std::lock_guard lck(socketMtx);
		visitor->receive(dataArr);
	}
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
			StreamTag type;
			strm >> handle >> type;
			auto nativeHandle = nativeDevice->create_stream(type);
			vstd::optional<RemoteStream>* stream;
			stream = &streamMap.Emplace(handle).Value();
			stream->New(nativeDevice, this, handle, nativeHandle);
		} break;
		case SerTag::DestroyStream: {
			uint64 handle;
			strm >> handle;
			auto ite = streamMap.Find(handle);
			assert(ite);
			streamMap.Remove(ite);
		} break;
		case SerTag::SyncStream: {
			uint64 handle;
			strm >> handle;
			auto stream = GetStream(handle);
			nativeDevice->synchronize_stream(stream->nativeHandle);
		} break;
		case SerTag::DispatchCommandList: {
			uint64 handle;
			strm >> handle;
			auto stream = GetStream(handle);
			stream->Dispatch(strm);
		} break;
		case SerTag::CreateShader: {
			uint64 handle, funcHash;
			strm >> handle >> funcHash;
			auto funcIte = funcMap.Find(funcHash);
			assert(funcIte);
			Function func(&funcIte.Value());
			uint64 strLen;
			strm >> strLen;
			vstd::string fileName = GetStr(strLen);
			auto nativeHandle = nativeDevice->create_shader(func, fileName);
			{
				std::lock_guard lck(handleMtx);
				handleMap.Emplace(handle, nativeHandle);
			}
		} break;
		case SerTag::DestroyShader: {
			DEVICEDESER_DESTROY(destroy_shader)
		} break;
		case SerTag::LoadShader: {
			uint64 handle, strLen;
			strm >> handle >> strLen;
			vstd::string fileName = GetStr(strLen);
			uint64 typeCount;
			strm >> typeCount;
			vstd::vector<Type const*> types;
			types.push_back_func(
				typeCount,
				[&] -> Type const* {
					uint64 strLen;
					strm >> strLen;
					vstd::string typeDesc = GetStr(strLen);
					return Type::from(typeDesc);
				});
			auto nativeHandle = nativeDevice->load_shader(fileName, types);
			{
				std::lock_guard lck(handleMtx);
				handleMap.Emplace(handle, nativeHandle);
			}
		} break;
		case SerTag::CreateGpuEvent: {
			uint64 handle;
			strm >> handle;
			auto nativeHandle = nativeDevice->create_event();
			{
				std::lock_guard lck(handleMtx);
				handleMap.Emplace(handle, nativeHandle);
			}
		} break;
		case SerTag::DestroyGpuEvent: {
			DEVICEDESER_DESTROY(destroy_event)
		} break;
		case SerTag::SignalGpuEvent: {
			uint64 handle, streamHandle;
			strm >> handle >> streamHandle;
			handle = HandleFilter(handle);
			streamHandle = HandleFilter(streamHandle);
			nativeDevice->signal_event(handle, streamHandle);
		}
		case SerTag::WaitGpuEvent: {
			uint64 handle, streamHandle;
			strm >> handle >> streamHandle;
			handle = HandleFilter(handle);
			streamHandle = HandleFilter(streamHandle);
			nativeDevice->wait_event(handle, streamHandle);
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
			builder = &funcMap.Emplace(hash, Function::Tag::KERNEL).Value();
			ser.DeserializeKernel(builder, db->GetRootNode());
			break;
		}
	}
}

RemoteStream* DeviceDeser::GetStream(uint64 handle) const {
	auto ite = streamMap.Find(handle);
	assert(ite);
	return ite.Value();
}

RemoteStream::RemoteStream(Device::Interface* device, DeviceDeser* deserDevice, uint64 handle, uint64 nativeHandle)
	: device(device), nativeHandle(nativeHandle), handle(handle) {
	visitor.deserDevice = deserDevice;
}
uint64 RemoteStream::DeviceSearchVisitor::GetResource(uint64 handle) {
	std::shared_lock lck(deserDevice->handleMtx);
	auto ite = deserDevice->handleMap.Find(handle);
	assert(ite);
	return ite.Value();
}
Function RemoteStream::DeviceSearchVisitor::GetFunction(uint64 handle) {
	auto ite = deserDevice->funcMap.Find(handle);
	assert(ite);
	return Function(&ite.Value());
}
RemoteStream::~RemoteStream() {}
void RemoteStream::Dispatch(ArrayOStream& strm) {
	CommandList list;
	CmdDeser cmdDeser;
	cmdDeser.visitor = &visitor;
	cmdDeser.arr = &strm;
	auto&& vec = cmdDeser.readbackDatas;
	cmdDeser.DeserCommands(sizeof(uint64), list);
	memcpy(vec.data(), &handle, sizeof(uint64));
	device->dispatch(nativeHandle, std::move(list));
	device->dispatch(nativeHandle, [this, readback = std::move(vec)]() mutable {
		std::lock_guard lck(visitor.deserDevice->socketMtx);
		visitor.deserDevice->socket->send_async(std::move(readback));
	});
}
#undef DEVICEDESER_DESTROY
}// namespace luisa::compute