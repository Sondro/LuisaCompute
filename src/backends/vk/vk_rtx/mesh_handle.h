#pragma once
#include <vstl/Common.h>
namespace toolhub::vk {
class Mesh;
class Accel;
class MeshHandle {
public:
	Mesh* mesh;
	Accel* accel;
	size_t accelIndex;
	size_t meshIndex;
	static MeshHandle* AllocateHandle();
	static void DestroyHandle(MeshHandle* handle);
};
}// namespace toolhub::vk