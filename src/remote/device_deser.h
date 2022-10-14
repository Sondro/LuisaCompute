#pragma once
#include <vstl/Common.h>
#include <runtime/device.h>
#include "array_iostream.h"
#include "socket_visitor.h"
#include <vstl/LockFreeArrayQueue.h>
#include <remote/DatabaseInclude.h>
#include <shared_mutex>
#include "cmd_deser.h"
namespace luisa::compute {
class DeviceDeser;
struct RemoteStream {
	struct DeviceSearchVisitor : public CmdDeser::IHandleMapVisitor {
		DeviceDeser* deserDevice;
		uint64 GetResource(uint64 handle) override;
		Function GetFunction(uint64 handle) override;
	};
	Device::Interface* device;
	uint64 handle;
	uint64 nativeHandle;
	DeviceSearchVisitor visitor;
	RemoteStream(Device::Interface* device, DeviceDeser* deserDevice, uint64 handle, uint64 nativeHandle);
	~RemoteStream();
	RemoteStream(RemoteStream const&) = delete;
	RemoteStream(RemoteStream&&) = delete;
	void Dispatch(ArrayOStream& strm);

};
class DeviceDeser : public vstd::IOperatorNewBase {
	friend class RemoteStream::DeviceSearchVisitor;
	friend class RemoteStream;
	SocketVisitor* socket;
	luisa::vector<std::byte> dataArr;
	Device::Interface* nativeDevice{nullptr};
	mutable std::shared_mutex handleMtx;
	mutable std::mutex socketMtx;
	vstd::HashMap<uint64, uint64> handleMap;
	vstd::HashMap<uint64, vstd::optional<RemoteStream>> streamMap;
	vstd::HashMap<uint64, detail::FunctionBuilder> funcMap;
	uint64 HandleFilter(uint64 handle) const;
	RemoteStream* GetStream(uint64 handle) const;

public:
	vstd::function<Device::Interface*()> createDeviceFunc;
	vstd::function<void(Device::Interface*)> destroyDeviceFunc;
	void Receive(SocketVisitor* visitor);
};
}// namespace luisa::compute