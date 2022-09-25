#pragma once
#include <stdint.h>
namespace luisa::compute {
enum class SerTag : uint32_t {
	CreateDevice,
	DestroyDevice,
	CreateBuffer,
	DestroyBuffer,
	CreateTexture,
	DestroyTexture,
	CreateBindlessArray,
	DestroyBindlessArray,
	EmplaceBufferInBindless,
	EmplaceTex2DInBindless,
	EmplaceTex3DInBindless,
	RemoveBufferInBindless,
	RemoveTex2DInBindless,
	RemoveTex3DInBindless,
	CreateStream,
	DestroyStream,
	SyncStream,
	DispatchCommandList,
	CreateShader,
	DestroyShader,
	LoadShader,
	CreateGpuEvent,
	DestroyGpuEvent,
	SignalGpuEvent,
	WaitGpuEvent,
	CreateMesh,
	DestroyMesh,
	CreateAccel,
	DestroyAccel,
	TransportFunction
};
}// namespace luisa::compute