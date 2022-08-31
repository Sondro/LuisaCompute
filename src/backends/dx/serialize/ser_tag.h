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
	Dispatch,
	CreateShader,
	DestroyShader,
	LoadShader,
	CreateEvent,
	DestroyEvent,
	SignalEvent,
	WaitEvent,
	SyncEvent,
	CreateMesh,
	DestroyMesh,
	CreateAccel,
	DestroyAccel
};
}// namespace luisa::compute