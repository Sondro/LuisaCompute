#include "mesh_handle.h"
#include "mesh.h"
#include "accel.h"
namespace toolhub::vk {
namespace detail {
static vstd::Pool<MeshHandle> meshHandlePool(32, false);
static vstd::spin_mutex meshHandleMtx;
}// namespace detail
MeshHandle* MeshHandle::AllocateHandle() {
	using namespace detail;
	return meshHandlePool.New_Lock(meshHandleMtx);
}
void MeshHandle::DestroyHandle(MeshHandle* handle) {
	using namespace detail;
	meshHandlePool.Delete_Lock(meshHandleMtx, handle);
}
}// namespace toolhub::vk